/// @file
/// @author uentity
/// @date 22.06.2018
/// @brief Implementation of BS tree-related serialization
/// @copyright
/// This Source Code Form is subject to the terms of the Mozilla Public License,
/// v. 2.0. If a copy of the MPL was not distributed with this file,
/// You can obtain one at https://mozilla.org/MPL/2.0/

#include <bs/actor_common.h>
#include <bs/atoms.h>
#include <bs/kernel/radio.h>
#include <bs/log.h>
#include <bs/serialize/serialize.h>
#include <bs/serialize/tree.h>
#include <bs/serialize/cafbind.h>

CAF_ALLOW_UNSAFE_MESSAGE_TYPE(blue_sky::tree::on_serialized_f)
CAF_ALLOW_UNSAFE_MESSAGE_TYPE(blue_sky::tree::link)
CAF_ALLOW_UNSAFE_MESSAGE_TYPE(blue_sky::error::box)

NAMESPACE_BEGIN(blue_sky::tree)
using namespace kernel::radio;
/*-----------------------------------------------------------------------------
 *  tree serialization actor impl
 *-----------------------------------------------------------------------------*/
NAMESPACE_BEGIN()
///////////////////////////////////////////////////////////////////////////////
//  Tree FS archive
//
auto save_fs(const link& root, const std::string& filename) -> error {
	// collect all errors happened
	auto errs = std::vector<error>{};
	if(auto er = error::eval_safe([&] {
		auto ar = tree_fs_output(filename);
		ar(root);
		errs = ar.wait_objects_saved(infinite);
	}))
		errs.push_back(er);

	std::string reduced_er;
	for(const auto& er : errs) {
		if(er) {
			if(!reduced_er.empty()) reduced_er += '\n';
			reduced_er += er.what();
		}
	}
	return reduced_er.empty() ? success() : error::quiet(reduced_er);
}

auto load_fs(link& root, const std::string& filename) -> error {
	return error::eval_safe([&] {
		auto ar = tree_fs_input(filename, tree_fs_input::NodeLoad::Normal);
		ar(root);
		ar.serializeDeferments();
	});
}

///////////////////////////////////////////////////////////////////////////////
//  generic archives
//
auto save_generic(const link& root, const std::string& filename, TreeArchive ar) -> error {
return error::eval_safe([&]() -> error {
	// open file for writing
	std::ofstream fs(
		filename,
		std::ios::out | std::ios::trunc | (ar == TreeArchive::Binary ? std::ios::binary : std::ios::openmode())
	);
	fs.exceptions(fs.failbit | fs.badbit);

	// dump link to JSON archive
	if(ar == TreeArchive::Binary) {
		cereal::PortableBinaryOutputArchive ja(fs);
		ja(root);
	}
	else {
		cereal::JSONOutputArchive ja(fs);
		ja(root);
	}
	return perfect;
}); }

auto load_generic(link& root, const std::string& filename, TreeArchive ar) -> error {
return error::eval_safe([&]() -> error {
	// open file for reading
	std::ifstream fs(
		filename,
		std::ios::in | (ar == TreeArchive::Binary ? std::ios::binary : std::ios::openmode())
	);
	fs.exceptions(fs.failbit | fs.badbit);

	// load link from JSON archive
	if(ar == TreeArchive::Binary) {
		cereal::PortableBinaryInputArchive ja(fs);
		ja(root);
	}
	else {
		cereal::JSONInputArchive ja(fs);
		ja(root);
	}
	return perfect;
}); }

///////////////////////////////////////////////////////////////////////////////
//  serial ator impl
//
auto save_actor(
	caf::event_based_actor* self, link root, std::string filename, TreeArchive ar,
	on_serialized_f cb
) -> caf::behavior {
	return {
		[ar, r = std::move(root), filename = std::move(filename), cb = std::move(cb)](a_apply) mutable
		-> error::box {
			// launch work
			auto er = ar == TreeArchive::FS ? save_fs(r, filename) : save_generic(r, filename, ar);
			// invoke callback
			if(cb) error::eval_safe([&]{ cb(std::move(r), er); });
			return er;
		}
	};
}

auto load_actor(
	caf::event_based_actor* self, std::string filename, TreeArchive ar,
	on_serialized_f cb
) -> caf::behavior {
	return {
		[ar, filename = std::move(filename), cb = std::move(cb)](a_apply) mutable
		-> result_or_errbox<link> {
			// launch work
			link r;
			auto er = ar == TreeArchive::FS ? load_fs(r, filename) : load_generic(r, filename, ar);
			// invoke callback
			if(cb) error::eval_safe([&]{ cb(r, er); });
			if(er.ok())
				return r;
			else
				return tl::make_unexpected(std::move(er));
		}
	};
}

NAMESPACE_END()

/*-----------------------------------------------------------------------------
 *  tree save impl
 *-----------------------------------------------------------------------------*/
auto save_tree(link root, std::string filename, TreeArchive ar, timespan wait_for) -> error {
	bsout() << "*** save_tree with {} timeout" <<
		(wait_for == infinite ? "infinite" : to_string(wait_for)) << bs_end;
	// launch worker in detached actor
	auto Af = caf::make_function_view(
		system().spawn<caf::spawn_options::detach_flag>(
			save_actor, std::move(root), std::move(filename), ar, on_serialized_f{}
		),
		wait_for == infinite ? caf::infinite : caf::duration{wait_for}
	);

	// wait for result
	return actorf<error>(Af, a_apply());
}

BS_API auto save_tree(
	on_serialized_f cb, link root, std::string filename, TreeArchive ar
) -> void {
	// [NOTE] spawn save in detached actor to prevent starvation
	auto A = system().spawn<caf::spawn_options::detach_flag>(
		save_actor, std::move(root), std::move(filename), ar, std::move(cb)
	);
	caf::anon_send(A, a_apply());
}

/*-----------------------------------------------------------------------------
 *  tree load impl
 *-----------------------------------------------------------------------------*/
auto load_tree(std::string filename, TreeArchive ar) -> result_or_err<link> {
	link r;
	auto er = ar == TreeArchive::FS ? load_fs(r, filename) : load_generic(r, filename, ar);
	if(er.ok())
		return r;
	else
		return tl::make_unexpected(std::move(er));

	//// launch worker in detached actor
	//auto Af = caf::make_function_view(
	//	system().spawn<caf::spawn_options::detach_flag>(
	//		load_actor, std::move(filename), ar, on_serialized_f{}
	//	),
	//	caf::infinite
	//);

	//// wait for result
	//return actorf<result_or_errbox<link>>(Af, a_apply());
}

auto load_tree(
	on_serialized_f cb, std::string filename, TreeArchive ar
) -> void {
	// [NOTE] spawn save in detached actor to prevent starvation
	auto A = system().spawn<caf::spawn_options::detach_flag>(
		load_actor, std::move(filename), ar, std::move(cb)
	);
	caf::anon_send(A, a_apply());
}

NAMESPACE_END(blue_sky::tree)
