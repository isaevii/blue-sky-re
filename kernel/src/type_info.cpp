/// @file
/// @author uentity
/// @date 28.04.2016
/// @brief 
/// @copyright
/// This Source Code Form is subject to the terms of the Mozilla Public License,
/// v. 2.0. If a copy of the MPL was not distributed with this file,
/// You can obtain one at https://mozilla.org/MPL/2.0/

#include <bs/type_info.h>
#include <bs/type_descriptor.h>

constexpr auto BS_NIL_TYPE_TAG = "__blue_sky_nil_type__";

namespace blue_sky {
/*-----------------------------------------------------------------------------
 *  Nil type tag
 *-----------------------------------------------------------------------------*/
class nil {};

const std::type_index& nil_type_info() {
	static const auto nil_ti = BS_GET_TI(nil);
	return nil_ti;
}

const std::string& nil_type_name() {
	static const auto nil_name = std::string(BS_NIL_TYPE_TAG);
	return nil_name;
}

bool is_nil(const std::type_index& t) {
	return t == std::type_index(typeid(nil));
}

/*-----------------------------------------------------------------------------
 *  type_descriptor impl
 *-----------------------------------------------------------------------------*/
// constructor from string type name for temporary tasks (searching etc)
type_descriptor::type_descriptor(std::string_view type_name) :
	parent_td_fun_(&nil), copy_fun_(nullptr), name(type_name)
{}

// standard constructor
type_descriptor::type_descriptor(
	std::string type_name, const BS_TYPE_COPY_FUN& cp_fn,
	const BS_GET_TD_FUN& parent_td_fn, std::string description
) :
	parent_td_fun_(parent_td_fn), copy_fun_(cp_fn),
	name(std::move(type_name)), description(std::move(description))
{}

// obtain Nil type_descriptor instance
const type_descriptor& type_descriptor::nil() {
	static const auto nil_td = type_descriptor(
		nil_type_name(), nullptr, &nil, "Nil type descriptor"
	);
	return nil_td;
}

bool type_descriptor::is_nil() const {
	return parent_td_fun_ == &nil;
}

bool type_descriptor::operator <(const type_descriptor& td) const {
	return name < td.name;
}

bool upcastable_eq::operator()(const type_descriptor& td1, const type_descriptor& td2) const {
	if(td1 == td2) return true;

	const type_descriptor* cur_td = &td2.parent_td();
	while(!cur_td->is_nil()) {
		if(td1 == *cur_td)
			return true;
		cur_td = &cur_td->parent_td();
	}
	return false;
}

} // eof blue_sky namespace

