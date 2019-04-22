/// @file
/// @author uentity
/// @date 22.04.2019
/// @brief BS tree links bindings
/// @copyright
/// This Source Code Form is subject to the terms of the Mozilla Public License,
/// v. 2.0. If a copy of the MPL was not distributed with this file,
/// You can obtain one at https://mozilla.org/MPL/2.0/

#include <bs/python/common.h>
#include <bs/python/tree.h>
#include <bs/python/expected.h>
#include <bs/tree/link.h>
#include <bs/tree/node.h>

#include <boost/uuid/uuid_io.hpp>
#include <pybind11/functional.h>
#include <pybind11/chrono.h>

NAMESPACE_BEGIN(blue_sky::python)

void py_bind_link(py::module& m) {
	using namespace tree;

	// inode binding
	py::class_<inode>(m, "inode")
		.def_readonly("owner", &inode::owner)
		.def_readonly("group", &inode::group)
		.def_readonly("mod_time", &inode::mod_time)
		//.def_property_readonly("mod_time", [](const inode& i) {
		//	return std::chrono::time_point_cast<std::chrono::system_clock::duration>(i.mod_time);
		//})
		.def_property_readonly("flags", [](const inode& i) { return i.flags; })
		.def_property_readonly("u", [](const inode& i) { return i.u; })
		.def_property_readonly("g", [](const inode& i) { return i.g; })
		.def_property_readonly("o", [](const inode& i) { return i.o; })
	;

	///////////////////////////////////////////////////////////////////////////////
	//  Base link
	//
	py::class_<link, py_link<>, std::shared_ptr<link>> link_pyface(m, "link");

	// export Flags enum
	py::enum_<link::Flags>(link_pyface, "Flags", py::arithmetic())
		.value("Persistent", link::Flags::Persistent)
		.value("Disabled", link::Flags::Disabled)
		.export_values()
	;
	py::implicitly_convertible<int, link::Flags>();
	py::implicitly_convertible<long, link::Flags>();

	// export link request & status enums
	py::enum_<link::Req>(link_pyface, "Req")
		.value("Data", link::Req::Data)
		.value("DataNode", link::Req::DataNode)
	;
	py::enum_<link::ReqStatus>(link_pyface, "ReqStatus")
		.value("Void", link::ReqStatus::Void)
		.value("Busy", link::ReqStatus::Busy)
		.value("OK", link::ReqStatus::OK)
		.value("Error", link::ReqStatus::Error)
	;

	// link base class
	link_pyface
		.def("clone", &link::clone, "deep"_a = true, "Make shallow or deep copy of link")
		.def("data_ex", &link::data_ex, "wait_if_busy"_a = true)
		.def("data", py::overload_cast<>(&link::data, py::const_))
		.def("data", py::overload_cast<link::process_data_cb, bool>(&link::data, py::const_),
			"f"_a, "high_priority"_a = false
		)
		.def("data_node_ex", &link::data_node_ex, "wait_if_busy"_a = true)
		.def("data_node", py::overload_cast<>(&link::data_node, py::const_))
		.def("data_node", py::overload_cast<link::process_data_cb, bool>(&link::data_node, py::const_),
			"f"_a, "high_priority"_a = false
		)
		.def("type_id", &link::type_id)
		.def("oid", &link::oid)
		.def("obj_type_id", &link::obj_type_id)
		.def("rename", &link::rename)
		.def("req_status", &link::req_status, "Query for given operation status")
		.def("rs_reset", &link::rs_reset,
			"request"_a, "new_status"_a = link::ReqStatus::Void,
			"Unconditionally set status of given request"
		)
		.def("rs_reset_if_eq", &link::rs_reset_if_eq,
			"request"_a, "self_rs"_a, "new_rs"_a = link::ReqStatus::Void,
			"Set status of given request if it is equal to given value, returns prev status"
		)
		.def("rs_reset_if_neq", &link::rs_reset_if_neq,
			"request"_a, "self_rs"_a, "new_rs"_a = link::ReqStatus::Void,
			"Set status of given request if it is NOT equal to given value, returns prev status"
		)

		.def_property_readonly("id", [](const link& L) {
			return boost::uuids::to_string(L.id());
		})
		.def_property_readonly("name", &link::name)
		.def_property_readonly("owner", &link::owner)
		.def_property_readonly("info", &link::info)
		.def_property("flags", &link::flags, &link::set_flags)
	;

	///////////////////////////////////////////////////////////////////////////////
	//  Derived links
	//
	py::class_<hard_link, link, py_link<hard_link>, std::shared_ptr<hard_link>>(m, "hard_link")
		.def(py::init<std::string, sp_obj, link::Flags>(),
			"name"_a, "data"_a, "flags"_a = link::Flags::Plain)
	;

	py::class_<weak_link, link, py_link<weak_link>, std::shared_ptr<weak_link>>(m, "weak_link")
		.def(py::init<std::string, const sp_obj&, link::Flags>(),
			"name"_a, "data"_a, "flags"_a = link::Flags::Plain)
	;

	py::class_<sym_link, link, py_link<sym_link>, std::shared_ptr<sym_link>>(m, "sym_link")
		.def(py::init<std::string, std::string, link::Flags>(),
			"name"_a, "path"_a, "flags"_a = link::Flags::Plain)
		.def(py::init<std::string, const sp_link&, link::Flags>(),
			"name"_a, "source"_a, "flags"_a = link::Flags::Plain)

		.def_property_readonly("is_alive", &sym_link::is_alive)
		.def("src_path", &sym_link::src_path, "human_readable"_a = false)
	;

	///////////////////////////////////////////////////////////////////////////////
	//  fusion link/iface
	//
	py::class_<fusion_link, link, py_link<fusion_link>, std::shared_ptr<fusion_link>>(m, "fusion_link")
		.def(py::init<std::string, sp_node, sp_fusion, link::Flags>(),
			"name"_a, "data"_a, "bridge"_a = nullptr, "flags"_a = link::Flags::Plain)
		.def(py::init<std::string, const char*, std::string, sp_fusion, link::Flags>(),
			"name"_a, "obj_type"_a, "oid"_a = "", "bridge"_a = nullptr, "flags"_a = link::Flags::Plain)
		.def_property("bridge", &fusion_link::bridge, &fusion_link::reset_bridge)
		.def("populate",
			py::overload_cast<const std::string&, bool>(&fusion_link::populate, py::const_),
			"child_type_id"_a, "wait_if_busy"_a = true
		)
		.def("populate",
			py::overload_cast<link::process_data_cb, std::string>(&fusion_link::populate, py::const_),
			"f"_a, "obj_type_id"_a
		)
	;

	py::class_<fusion_iface, py_fusion<>, std::shared_ptr<fusion_iface>>(m, "fusion_iface")
		.def("populate", &fusion_iface::populate, "root"_a, "child_type_id"_a = "",
			"Populate root object structure (children)")
		.def("pull_data", &fusion_iface::pull_data, "root"_a,
			"Fill root object content")
	;
}

NAMESPACE_END(blue_sky::python)