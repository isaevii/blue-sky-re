/// @file
/// @author uentity
/// @date 24.08.2016
/// @brief Plugins subsystem of BS kernel
/// @copyright
/// This Source Code Form is subject to the terms of the Mozilla Public License,
/// v. 2.0. If a copy of the MPL was not distributed with this file,
/// You can obtain one at https://mozilla.org/MPL/2.0/

#ifdef BSPY_EXPORTING
#include <bs/python/common.h>
#endif

#include <bs/error.h>
#include <bs/kernel/errors.h>
#include <bs/log.h>
#include <bs/detail/scope_guard.h>

#include "plugins_subsyst.h"
#include "python_subsyst.h"

#define BS_KERNEL_VERSION "0.1" //!< version of blue-sky kernel

#define PYSS singleton<python_subsyst>::Instance()

/*-----------------------------------------------------------------------------
 *  BS kernel plugin descriptor
 *-----------------------------------------------------------------------------*/
BS_C_API blue_sky::plugin_descriptor* bs_get_plugin_descriptor() {
	return &blue_sky::kernel::detail::plugins_subsyst::kernel_pd();
}

NAMESPACE_BEGIN(blue_sky::kernel::detail)

// hide implementation
NAMESPACE_BEGIN()

// tags for kernel & runtime types plugin_descriptors
struct __kernel_types_pd_tag__ {};
struct __runtime_types_pd_tag__ {};

NAMESPACE_END() // eof hidden namespace

// init kernel plugin descriptors
auto plugins_subsyst::kernel_pd() -> plugin_descriptor& {
	static plugin_descriptor kernel_pd(
		BS_GET_TI(__kernel_types_pd_tag__), "kernel", BS_KERNEL_VERSION,
		"BlueSky virtual kernel plugin", "bs",
		(void*)&cereal::detail::StaticObject<cereal::detail::InputBindingMap>::getInstance(),
		(void*)&cereal::detail::StaticObject<cereal::detail::OutputBindingMap>::getInstance()
	);
	return kernel_pd;
}

auto plugins_subsyst::runtime_pd() -> plugin_descriptor& {
	static plugin_descriptor runtime_pd(
		BS_GET_TI(__runtime_types_pd_tag__), "runtime", BS_KERNEL_VERSION,
		"BlueSky virtual plugin for runtime types", "bs"
	);
	return runtime_pd;
}

plugins_subsyst::plugins_subsyst() {
	// register kernel virtual plugins
	register_plugin(&kernel_pd(), lib_descriptor());
	register_plugin(&runtime_pd(), lib_descriptor());
}

plugins_subsyst::~plugins_subsyst() {}

std::pair< const plugin_descriptor*, bool >
plugins_subsyst::register_plugin(const plugin_descriptor* pd, const lib_descriptor& ld) {
	// deny registering nil plugin descriptors
	if(!pd) return {&plugin_descriptor::nil(), false};

	// find or insert passed plugin_descriptor
	auto res = loaded_plugins_.emplace(pd, ld);
	auto pplug = res.first->first;

	// if plugin with same name was already registered
	if(!res.second) {
		if(pplug->is_nil() && !pd->is_nil()) {
			// replace temp nil plugin with same name
			loaded_plugins_.erase(res.first);
			res = loaded_plugins_.emplace(pd, ld);
			pplug = res.first->first;
		}
		else if(!res.first->second.handle_ && ld.handle_) {
			// update lib descriptor to valid one
			res.first->second = ld;
			return {pplug, true};
		}
	}

	// for newly inserted valid (non-nill) plugin descriptor
	// update types that previousely referenced it by name
	if(res.second && !pplug->is_nil()) {
		auto& plug_name_view = types_.get< plug_name_key >();
		auto ptypes = plug_name_view.equal_range(pplug->name);
		for(auto ptype = ptypes.first; ptype != ptypes.second; ++ptype) {
			plug_name_view.replace(ptype, type_tuple{*pplug, ptype->td()});
		}

		// remove entry from temp plugins if any
		// temp plugins are always nill
		auto tplug = temp_plugins_.find(*pplug);
		if(tplug != temp_plugins_.end())
			temp_plugins_.erase(tplug);

		// merge serialization bindings maps
		unify_serialization();
	}

	return {pplug, res.second};
}

void plugins_subsyst::clean_plugin_types(const plugin_descriptor& pd) {
	// we cannot clear kernel internal types
	if(pd == kernel_pd()) return;

	types_.get< plug_name_key >().erase(pd.name);
}

// unloas given plugin
void plugins_subsyst::unload_plugin(const plugin_descriptor& pd) {
	// check if given plugin was registered
	auto plug = loaded_plugins_.find(&pd);
	if(plug == loaded_plugins_.end()) return;

	clean_plugin_types(pd);
	// unload and erase plugin
	plug->second.unload();
	loaded_plugins_.erase(plug);
}

// unloads all plugins
void plugins_subsyst::unload_plugins() {
	for(const auto& p : loaded_plugins_) {
		unload_plugin(*p.first);
	}
}

auto plugins_subsyst::load_plugin(const std::string& fname) -> error {
	using namespace std;

	// DLL handle
	lib_descriptor lib;
	auto unload_on_error = scope_guard{ [&lib]{ lib.unload(); } };

	static auto who = "load_plugins";
	plugin_initializer plugin_init;
	BS_GET_PLUGIN_DESCRIPTOR bs_plugin_descriptor;
	bs_register_plugin_fn bs_register_plugin;
	plugin_descriptor* p_descr = nullptr;

	try {
		// load library
		lib.load(fname.c_str());

		// check for plugin descriptor presence
		lib.load_sym("bs_get_plugin_descriptor", bs_plugin_descriptor);
		if(!bs_plugin_descriptor)
			return {lib.fname_, kernel::Error::BadBSplugin};

		// retrieve descriptor from plugin
		if(!(p_descr = dynamic_cast< plugin_descriptor* >(bs_plugin_descriptor())))
			return {lib.fname_, kernel::Error::BadPluginDescriptor};

		// check if loaded lib is really a blue-sky kernel
		if(*p_descr == kernel_pd())
			return error::quiet("load_plugin: cannot load kernel (already loaded)");

		// check if bs_register_plugin function present in library
		lib.load_sym("bs_register_plugin", bs_register_plugin);
		if(!bs_register_plugin)
			return {lib.fname_ + ": missing bs_register_plugin)", kernel::Error::BadBSplugin};

		// check if plugin was already registered earlier
		if(!register_plugin(p_descr, lib).second)
			return {lib.fname_, kernel::Error::PluginAlreadyRegistered};

		// TODO: enable versions checking
		// check version
		//if(version.size() && version_comparator(p_descr->version.c_str(), version.c_str()) < 0) {
		//	delay_unload killer(&lib, LU);
		//	bsout() << log::E("{}: BlueSky plugin {} has wrong version") << who << lib.fname_ << log::end;
		//	return retval;
		//}

		// invoke bs_register_plugin
		plugin_init.pd = p_descr;
		if(!bs_register_plugin(plugin_init)) {
			unload_plugin(*p_descr);
			return {lib.fname_, kernel::Error::PluginRegisterFail};
		}

		// init Python subsystem
		auto py_scope = PYSS.py_init_plugin(lib, *p_descr).value_or("");

		// print status
		std::string msg = "BlueSky plugin {} loaded";
		if(py_scope.size())
			msg += ", Python subsystem initialized, namespace: {}";
		auto log_msg = bsout() << log::I(msg.c_str()) << lib.fname_;
		if(py_scope.size())
			log_msg << py_scope << log::end;
		else
			log_msg << log::end;
	}
	catch(const error& ex) {
		return ex;
	}
	catch(const std::exception& ex) {
		return error(ex.what());
	}
	catch(...) {
		BSERROR << log::E("[Unknown Exception] {}: Unknown error happened during plugins loading")
			<< who << bs_end;
		throw;
	}

	// don't unload sucessfully loaded plugin
	unload_on_error.disable();
	return success();
}

auto plugins_subsyst::load_plugins() -> error {
	// discover plugins
	auto plugins = discover_plugins();
	BSOUT << "--------> [load plugins]" << bs_end;

	// collect error messages
	std::string err_messages;
	for(const auto& plugin_fname : plugins)
		if(auto er = load_plugin(plugin_fname)) {
			if(!err_messages.empty()) err_messages += '\n';
			err_messages += er.what();
		}

	// register closure ctor for error from any int value
	PYSS.py_add_error_closure();

	return err_messages.empty() ? success() : error::quiet(err_messages);
}

namespace {
// helper to unify input or output bindings
template<typename bindings_t>
auto unify_bindings(const plugins_subsyst& K, void *const plugin_descriptor::*binding_var) {
	using Serializers_map = typename bindings_t::Serializers_map;
	using Archives_map = typename bindings_t::Archives_map;
	Archives_map united;
	bindings_t* plug_bnd = nullptr;

	// lambda helper to merge bindings from source to destination
	auto merge_bindings = [](const auto& src, auto& dest) {
		// loop over archive entries
		for(const auto& ar : src) {
			auto ar_dest = dest.find(ar.first);
			// if entries for archive not found at all - insert 'em all at once
			// otherwise go deeper and loop over entries per selected archive
			if(ar_dest == dest.end())
				dest.insert(ar);
			else {
				// loop over bindings for selected archive
				// and insert bindings that are missing in destination
				auto& dest_binds = ar_dest->second;
				for(const auto& src_bind : ar.second) {
					if(dest_binds.find(src_bind.first) == dest_binds.end())
						dest_binds.insert(src_bind);
				}
			}
		}
	};

	// first pass -- collect all bindings into single map
	for(const auto& plugin : K.loaded_plugins_) {
		// extract bindings global from plugin descriptor
		if( !(plug_bnd = reinterpret_cast<bindings_t*>(plugin.first->*binding_var)) )
			continue;
		// merge plugin bindings into united map
		merge_bindings(plug_bnd->archives_map, united);
	}

	// 2nd pass - merge united into plugins' bindings
	for(const auto& plugin : K.loaded_plugins_) {
		// extract bindings global from plugin descriptor
		if( !(plug_bnd = reinterpret_cast<bindings_t*>(plugin.first->*binding_var)) )
			continue;
		// merge all entries from plugin into united
		merge_bindings(united, plug_bnd->archives_map);
	}
}

} // eof hidden namespace

void plugins_subsyst::unify_serialization() const {
	unify_bindings<cereal::detail::InputBindingMap>(
		*this, &plugin_descriptor::serial_input_bindings
	);
	unify_bindings<cereal::detail::OutputBindingMap>(
		*this, &plugin_descriptor::serial_output_bindings
	);
}

NAMESPACE_END(blue_sky::kernel::detail)
