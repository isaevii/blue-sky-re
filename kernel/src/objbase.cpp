/// @file
/// @author uentity
/// @date 05.03.2007
/// @brief Just BlueSky object base implimentation
/// @copyright
/// This Source Code Form is subject to the terms of the Mozilla Public License,
/// v. 2.0. If a copy of the MPL was not distributed with this file,
/// You can obtain one at https://mozilla.org/MPL/2.0/

#include <bs/objbase.h>
#include <bs/kernel.h>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

// -----------------------------------------------------
// Implementation of class: object_base
// -----------------------------------------------------

NAMESPACE_BEGIN(blue_sky)

namespace {

// global random UUID generator for BS objects
static boost::uuids::random_generator gen;

} // eof hidden namespace

objbase::objbase(std::string custom_oid)
	: objbase(false, custom_oid)
{}

objbase::objbase(bool is_node, std::string custom_oid)
	: id_(custom_oid.size() ? std::move(custom_oid) : boost::uuids::to_string(gen())),
	is_node_(is_node)
{}

objbase::objbase(const objbase& obj)
	: enable_shared_from_this(obj), id_(boost::uuids::to_string(gen())), is_node_(obj.is_node_)
{}

void objbase::swap(objbase& rhs) {
	std::swap(is_node_, rhs.is_node_);
	std::swap(id_, rhs.id_);
}

objbase::~objbase() {}


const type_descriptor& objbase::bs_type() {
	static type_descriptor td(
		identity< objbase >(), identity< nil >(),
		"objbase", "Base class of all BlueSky types", std::true_type(), std::true_type()
	);
	return td;
}

int objbase::bs_register_this() const {
	return BS_KERNEL.register_instance(shared_from_this());
}

int objbase::bs_free_this() const {
	return BS_KERNEL.free_instance(shared_from_this());
}

std::string objbase::type_id() const {
	return bs_resolve_type().name;
}

std::string objbase::id() const {
	return id_;
}

bool objbase::is_node() const {
	return is_node_;
}

NAMESPACE_END(blue_sky)

