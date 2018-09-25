/// @file
/// @author uentity
/// @date 24.08.2016
/// @brief BlueSky kernel_impl definition
/// @copyright
/// This Source Code Form is subject to the terms of the Mozilla Public License,
/// v. 2.0. If a copy of the MPL was not distributed with this file,
/// You can obtain one at https://mozilla.org/MPL/2.0/

#include <bs/kernel.h>
#include <bs/error.h>

#include <caf/actor_system_config.hpp>
#include <caf/actor_system.hpp>
#include <caf/io/middleman.hpp>

#include "kernel_logging_subsyst.h"
#include "kernel_plugins_subsyst.h"
#include "kernel_instance_subsyst.h"

namespace blue_sky {
/*-----------------------------------------------------------------------------
 *  kernel impl
 *-----------------------------------------------------------------------------*/
class kernel::kernel_impl : public detail::kernel_plugins_subsyst, public detail::kernel_instance_subsyst
{
public:
	// per-type kernel any arrays storage
	using str_any_map_t = std::map< BS_TYPE_INFO, str_any_array >;
	str_any_map_t str_any_map_;

	using idx_any_map_t = std::map< BS_TYPE_INFO, idx_any_array >;
	idx_any_map_t idx_any_map_;

	// kernel's actor system & config
	caf::actor_system_config actor_cfg_;

	kernel_impl() {
		// [TODO] implement actor system config parsing
		// load middleman module
		actor_cfg_.load<caf::io::middleman>();
		// [NOTE] We can't create `caf::actor_system` here.
		// `actor_system` starts worker and other service threads in constructor.
		// At the same time kernel singleton is constructed most of the time during
		// initialization of kernel shared library. And on Windows it is PROHIBITED to start threads
		// in `DllMain()`, because that cause a deadlock.
		// Solution: delay construction of actor_system until first usage, don't use CAf in kernel
		// ctor.
	}

	type_tuple find_type(const std::string& key) const {
		using search_key = detail::kernel_plugins_subsyst::type_name_key;

		auto tp = types_.get< search_key >().find(key);
		return tp != types_.get< search_key >().end() ? *tp : type_tuple();
	}

	// returns valid (non-nill) type info
	BS_TYPE_INFO find_type_info(const type_descriptor& master) const {
		using type_name_key = detail::kernel_plugins_subsyst::type_name_key;

		BS_TYPE_INFO info = master.type();
		if(is_nil(info)) {
			// try to find type info by type name
			info = find_type(master.name).td().type();
		}
		// sanity
		if(is_nil(info))
			throw error(
				fmt::format("Cannot find type info for type {}, seems like not registered", master.name)
			);
		return info;
	}

	static spdlog::logger& get_log(const char* name) {
		return detail::kernel_logging_subsyst::get_log(name);
	}

	str_any_array& pert_str_any_array(const type_descriptor& master) {
		return str_any_map_[find_type_info(master)];
	}

	idx_any_array& pert_idx_any_array(const type_descriptor& master) {
		return idx_any_map_[find_type_info(master)];
	}

	caf::actor_system& actor_system() {
		// delayed actor system initialization
		static auto actor_sys = std::make_unique<caf::actor_system>(actor_cfg_);
		return *actor_sys;
	}
};

} /* namespace blue_sky */

