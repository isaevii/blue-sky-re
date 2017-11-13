/// @file
/// @author uentity
/// @date 14.09.2016
/// @brief Implementation os link
/// @copyright
/// This Source Code Form is subject to the terms of the Mozilla Public License,
/// v. 2.0. If a copy of the MPL was not distributed with this file,
/// You can obtain one at https://mozilla.org/MPL/2.0/

#include <bs/link.h>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

NAMESPACE_BEGIN(blue_sky)
NAMESPACE_BEGIN(tree)

namespace {

// global random UUID generator for BS links
static auto gen = boost::uuids::random_generator();

} // eof hidden namespace

link::link(std::string name)
	: name_(std::move(name)),
	id_(gen()), flags_(0)
{}

// copy ctor does not copy uuid from lhs
// instead it creates a new one
link::link(const link& lhs)
	: std::enable_shared_from_this<link>(lhs), name_(lhs.name_), id_(gen()), flags_(0), owner_()
{}

link::~link() {}

// get link's object ID
std::string link::oid() const {
	auto obj = data();
	if(obj) return obj->id();
	return boost::uuids::to_string(boost::uuids::nil_uuid());
}

std::string link::obj_type_id() const {
	auto obj = data();
	if(obj) return obj->type_id();
	return type_descriptor::nil().name;
}

void link::reset_owner(const sp_node& new_owner) {
	owner_ = new_owner;
}

uint link::flags() const {
	return flags_;
}

void link::set_flags(Flags new_flags) {
	flags_ = new_flags;
}

NAMESPACE_END(tree)
NAMESPACE_END(blue_sky)

