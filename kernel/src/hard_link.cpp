/// @file
/// @author uentity
/// @date 14.09.2016
/// @brief hard link implementation
/// @copyright
/// This Source Code Form is subject to the terms of the Mozilla Public License,
/// v. 2.0. If a copy of the MPL was not distributed with this file,
/// You can obtain one at https://mozilla.org/MPL/2.0/

#include <bs/link.h>

NAMESPACE_BEGIN(blue_sky)

bs_hard_link::bs_hard_link(std::string name, const sp_obj& data) :
	bs_link(std::move(name)), data_(data)
{}

sp_obj bs_hard_link::data() const {
	return data_;
}

bs_link::link_type bs_hard_link::type_id() const {
	return hard_link;
}

sp_link bs_hard_link::clone() const {
	return std::make_shared< bs_hard_link >(*this);
}

NAMESPACE_END(blue_sky)
