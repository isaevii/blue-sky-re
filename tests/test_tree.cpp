/// @file
/// @author uentity
/// @date 29.06.2018
/// @brief Test cases for BS tree
/// @copyright
/// This Source Code Form is subject to the terms of the Mozilla Public License,
/// v. 2.0. If a copy of the MPL was not distributed with this file,
/// You can obtain one at https://mozilla.org/MPL/2.0/

#define BOOST_TEST_DYN_LINK

#include "test_objects.h"
#include "test_serialization.h"
#include <bs/kernel/kernel.h>
#include <bs/kernel/tools.h>
#include <bs/log.h>
#include <bs/tree/tree.h>

#include <bs/serialize/base_types.h>
#include <bs/serialize/array.h>
#include <bs/serialize/tree.h>

#include <boost/test/unit_test.hpp>
#include <iostream>
#include <caf/scoped_actor.hpp>

using namespace blue_sky;
using namespace blue_sky::log;
using namespace blue_sky::tree;

namespace {

class fusion_client : public fusion_iface {
	auto populate(const sp_node& root, const std::string& child_type_id = "") -> error override {
		bsout() << "fusion_client::populate() called" << end;
		return error::quiet();
	}

	auto pull_data(const sp_obj& root) -> error override {
		bsout() << "fusion_client::pull_data() called" << end;
		return error::quiet();
	}
};

} // eof hidden namespace

BOOST_AUTO_TEST_CASE(test_tree) {
	std::cout << "\n\n*** testing tree..." << std::endl;
	std::cout << "*********************************************************************" << std::endl;

	// person
	sp_obj P = kernel::tfactory::create_object(bs_person::bs_type(), std::string("Tyler"), double(33));
	// person link
	auto L = std::make_shared<hard_link>("person link", std::move(P));
	BOOST_TEST(L);
	auto L1 = test_json(L);
	BOOST_TEST(L->name() == L1->name());

	// create root link and node
	sp_node N = kernel::tfactory::create_object("node");
	// create several persons and insert 'em into node
	for(int i = 0; i < 10; ++i) {
		std::string p_name = "Citizen_" + std::to_string(i);
		N->insert(std::make_shared<hard_link>(
			p_name, kernel::tfactory::create_object("bs_person", p_name, double(i + 20))
		));
	}
	// create hard link referencing first object
	N->insert(std::make_shared<hard_link>(
		"hard_Citizen_0", N->begin()->get()->data()
	));
	// create weak link referencing 2nd object
	N->insert(std::make_shared<weak_link>(
		"weak_Citizen_1", N->find(1)->get()->data()
	));
	// create sym link referencing 3rd object
	N->insert(std::make_shared<sym_link>(
		"sym_Citizen_2", abspath(*N->find(2))
	));
	N->insert(std::make_shared<sym_link>(
		"sym_Citizen_3", abspath( deref_path(abspath(*N->find(3), node::Key::Name), N, node::Key::Name) )
	));
	N->insert(std::make_shared<sym_link>(
		"sym_dot", "."
	));
	// print resulting tree content
	auto hN = link::make_root<hard_link>("r", N);
	bsout() << "root node abspath: {}" << abspath(hN) << bs_end;
	bsout() << "root node abspath: {}" << convert_path(abspath(hN), hN, node::Key::ID, node::Key::Name) << bs_end;
	bsout() << "sym_Citizen_2 abspath: {}" << convert_path(
		abspath(*N->find("sym_Citizen_2", node::Key::Name)), hN, node::Key::ID, node::Key::Name
	) << bs_end;
	kernel::tools::print_link(hN, false);

	// serializze node
	auto N1 = test_json(N);

	// print loaded tree content
	kernel::tools::print_link(std::make_shared<hard_link>("r", N1), false);
	BOOST_TEST(N1);
	BOOST_TEST(N1->size() == N->size());

	// serialize to FS
	bsout() << "\n===========================\n" << bs_end;
	save_tree(hN, "tree_fs/.data", TreeArchive::FS);
	load_tree("tree_fs/.data", TreeArchive::FS).map([](const sp_link& hN1) {
		kernel::tools::print_link(hN1, false);
	});

	// test async dereference
	deref_path([](const sp_link& lnk) {
		std::cout << "*** Async deref callback: link : " <<
		(lnk ? abspath(lnk, node::Key::Name) : "None") << ' ' <<
		lnk->obj_type_id() << ' ' << (void*)lnk->data().get() << std::endl;
	}, "hard_Citizen_0", hN, node::Key::Name);

	// fusion link
	//{
	//	auto fl = std::make_shared<fusion_link>(
	//		"fuse1", nullptr, std::make_shared<fusion_client>()
	//	);
	//	//fl->test();
	//}
}

