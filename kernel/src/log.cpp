/// @file
/// @author uentity
/// @date 25.08.2016
/// @brief BlueSky logging implementation
/// @copyright
/// This Source Code Form is subject to the terms of the Mozilla Public License,
/// v. 2.0. If a copy of the MPL was not distributed with this file,
/// You can obtain one at https://mozilla.org/MPL/2.0/

#include <bs/log.h>

namespace blue_sky { namespace log {
using namespace spdlog;

/*-----------------------------------------------------------------
 * bs_log implementation
 *----------------------------------------------------------------*/
bs_log::bs_log(const char* name) : log_(get_logger(name)) {}

/*-----------------------------------------------------------------
 * manipulators implementation
 *----------------------------------------------------------------*/
bs_log& end(bs_log& l) {
	return l;
}

bs_log& infol(bs_log& l) {
	l.logger().set_level(level_enum::info);
	return l;
}

bs_log& warnl(bs_log& l) {
	l.logger().set_level(level_enum::warn);
	return l;
}

bs_log& errl(bs_log& l) {
	l.logger().set_level(level_enum::err);
	return l;
}

bs_log& critical(bs_log& l) {
	l.logger().set_level(level_enum::critical);
	return l;
}

bs_log& offl(bs_log& l) {
	l.logger().set_level(level_enum::off);
	return l;
}

bs_log& debugl(bs_log& l) {
	l.logger().set_level(level_enum::debug);
	return l;
}

bs_log& tracel(bs_log& l) {
	l.logger().set_level(level_enum::trace);
	return l;
}

} // eof namespace blue_sky::log

} /* namespace blue_sky */

namespace std {

// provide convenience overlaods for aout; implemented in logging.cpp
blue_sky::log::bs_log& endl(blue_sky::log::bs_log& o) {
	return o;
}

} // namespace std

