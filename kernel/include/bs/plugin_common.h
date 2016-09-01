/// @file
/// @author uentity
/// @date 12.01.2016
/// @brief 
/// @copyright
/// This Source Code Form is subject to the terms of the Mozilla Public License,
/// v. 2.0. If a copy of the MPL was not distributed with this file,
/// You can obtain one at https://mozilla.org/MPL/2.0/

#pragma once

#include "setup_common_api.h"
#include "type_info.h"
#include "fwd.h"

#include <string>

namespace blue_sky {
	
struct BS_API plugin_descriptor {
	std::string name;
	std::string version;
	std::string short_descr;
	std::string long_descr;
	std::string py_namespace;

	// nil plugin constructor
	plugin_descriptor();

	// ctor for searching in containers
	plugin_descriptor(const char* name);

	// standard ctor for using in plugins
	plugin_descriptor(
		const BS_TYPE_INFO& plugin_tag, const char* name, const char* version, const char* short_descr,
		const char* long_descr = "", const char* py_namespace = ""
	);

	// test if this is a nil plugin
	bool is_nil() const;

	// comparision operators
	bool operator <(const plugin_descriptor&) const;
	bool operator ==(const plugin_descriptor&) const;
	bool operator !=(const plugin_descriptor& lhs) const {
		return !(*this == lhs);
	}

private:
	friend class kernel;
	BS_TYPE_INFO tag_;
};

// TODO: later move to common.h
struct BS_API plugin_initializer {
	kernel* k; //!< Reference to blue-sky kernel.
	plugin_descriptor const* pd; //!< Pointer to descriptor of plugin being loaded
};

}	// end of blue_sky namespace

/*!
	\brief Plugin descriptor macro.
	This macro should appear only once in all your plugin project, somewhere in main.cpp.
	Never put it into header file!
	Plugin descriptor generated by this macro is seeked upon plugin loading. If it isn't found,
	your library won't be recognized as BlueSky plugin, so don't forget to declare it.
	BLUE_SKY_PLUGIN_DESCRIPTOR_EXT allows you to set Python namespace (scope) name for all classes
	exported to Python.

  \param tag = tag for class
	\param name = name of the plugin
	\param version = plugin version
	\param short_descr = short description of the plugin
	\param long_descr = long description of the plugin
*/
#define BLUE_SKY_PLUGIN_DESCRIPTOR_EXT(name, version, short_descr, long_descr, py_namespace) \
namespace {                                                                                  \
    class BS_HIDDEN_API_PLUGIN _bs_this_plugin_tag_ {};                                      \
}                                                                                            \
BS_C_API_PLUGIN const blue_sky::plugin_descriptor* bs_get_plugin_descriptor() {              \
    static ::blue_sky::plugin_descriptor *plugin_info_ = new ::blue_sky::plugin_descriptor(  \
        BS_GET_TI (_bs_this_plugin_tag_),                                                    \
        name, version, short_descr, long_descr, py_namespace                                 \
    );                                                                                       \
    return plugin_info_;                                                                     \
}

#define BLUE_SKY_PLUGIN_DESCRIPTOR(name, version, short_descr, long_descr) \
BLUE_SKY_PLUGIN_DESCRIPTOR_EXT(name, version, short_descr, long_descr, "")

//! type of get plugin descrptor pointer function
typedef blue_sky::plugin_descriptor* (*BS_GET_PLUGIN_DESCRIPTOR)();

/*!
\brief Plugin register function.

Write it into yours main cpp-file of dynamic library.
This is begining of function.\n
For example:\n
BLUE_SKY_REGISTER_PLUGIN_FUN {\n
return BLUE_SKY_REGISTER_OBJ(foo,create_foo);\n
}
*/
#define BLUE_SKY_REGISTER_PLUGIN_FUN \
BS_C_API_PLUGIN bool bs_register_plugin(const blue_sky::plugin_initializer& bs_init)

//!	type of plugin register function (in libs)
typedef bool (*BS_REGISTER_PLUGIN)(const blue_sky::plugin_initializer&);

#define BLUE_SKY_INIT_PY_FUN \
BS_C_API_PLUGIN void bs_init_py_subsystem()
typedef void (*bs_init_py_fn)();

