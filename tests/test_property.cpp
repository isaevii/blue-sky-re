/// @file
/// @author uentity
/// @date 10.03.2019
/// @brief Test BS properties
/// @copyright
/// This Source Code Form is subject to the terms of the Mozilla Public License,
/// v. 2.0. If a copy of the MPL was not distributed with this file,
/// You can obtain one at https://mozilla.org/MPL/2.0/

#define BOOST_TEST_DYN_LINK

#include <bs/log.h>
#include <bs/property.h>
#include <bs/propdict.h>

#include <map>
#include <boost/test/unit_test.hpp>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <caf/deep_to_string.hpp>

template<typename T, typename U>
constexpr auto equal_il(const T& rhs, std::initializer_list<U> il) {
	return std::equal(std::cbegin(rhs), std::cend(rhs), std::cbegin(il), std::cend(il));
}

BOOST_AUTO_TEST_CASE(test_property) {
	using namespace blue_sky;
	using namespace blue_sky::prop;
	using namespace std::literals;

	bsout() << "*** testing property..." << bs_end;

	bsout() << "Property size = {}, it carries {} alts, {} scalars"
		<< sizeof(property)
		<< std::variant_size_v<property::underlying_type> << prop::detail::scalar_ts_num << bs_end;

	// make property
	auto p = property("Hello"s);
	bsout() << "p value index = {}, value = {}" << p.index() << get<std::string>(p) << bs_end;
	// test integer scalars list
	auto intV = {42L, 24L, 27L};
	p.emplace<list_of<integer>>(intV);
	auto getV = get<list_of<integer>>(p);
	bsout() << "p value index = {}" << p.index() << bs_end;
	BOOST_TEST(equal_il(getV, intV));

	// test real scalars list
	auto realV = {42., 24., 27.};
	p.emplace<list_of<double>>(realV);
	bsout() << "p value index = {}" << p.index() << bs_end;
	BOOST_TEST(equal_il(get<list_of<double>>(p), realV));

	// should fail to compile:
	//auto simpleV = std::vector<int64_t>();
	//get<list_of<integer>>(simpleV);

	property p1{in_place_list<integer>, intV};
	BOOST_TEST(equal_il(get<list_of<integer>>(p1), intV));

	p = 42L;
	bsout() << "p value index = {}, value = {}" << p.index() << get<integer>(p) << bs_end;
	double pp = 0.;
	extract<integer>(p, pp);
	BOOST_TEST(pp != 0.);
	p = "Hello";
	pp = 0.;
	extract<integer>(p, pp);
	BOOST_TEST(pp == 0.);

	///////////////////////////////////////////////////////////////////////////////
	//  propdict
	//
	bsout() << "*** testing propdict..." << bs_end;
	std::map<const char*, std::string> fixt = {{"A", "test1"}, {"B", "test2"}};
	propdict P = {{"A", "test2"}, {"B", 2L}, {"C", 42.}, {"D", std::vector{2L, 3L, 4L}}};
	bsout() << "P = {}" << P << bs_end;
	P = fixt;
	bsout() << "P = {}" << P << bs_end;
	P = std::move(fixt);
	for(const auto& [_, v] : fixt) {
		BOOST_TEST(v.empty());
	}

	P.ss<integer>("E") = 142;
	BOOST_TEST(get<integer>(P, "E") == 142);
	BOOST_TEST(*get_if<integer>(&P, "E") == 142);

	// cant' extract non-existent value to integer
	integer a = 0;
	BOOST_TEST((!extract(P, "F", a) && a == 0));
	// subscript with default value passed in
	BOOST_TEST(equal_il(P.ss<list_of<real>>("F", realV), realV));
	// can't extract non-compatible types
	BOOST_TEST((!extract(P, "F", a) && a == 0));
	// ... but can to compatible
	BOOST_TEST((extract(P, "E", a) && a == 142));
	a = 0;
	a = P.ss("E");
	BOOST_TEST(a == 142);
	// test timstamp & timespan
	P.ss<timestamp>("now") = make_timestamp();
	P.ss<timespan>("now duration", get<timestamp>(P, "now") - std::chrono::system_clock::now());
	bsout() << "P = {}" << P << bs_end;
}

