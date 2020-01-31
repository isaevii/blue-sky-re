/// @file
/// @author uentity
/// @date 29.06.2018
/// @brief BS tree node implementation part of PIMPL
/// @copyright
/// This Source Code Form is subject to the terms of the Mozilla Public License,
/// v. 2.0. If a copy of the MPL was not distributed with this file,
/// You can obtain one at https://mozilla.org/MPL/2.0/
#pragma once

#include <bs/actor_common.h>
#include <bs/log.h>
#include <bs/tree/node.h>
#include <bs/detail/function_view.h>
#include <bs/detail/sharded_mutex.h>
#include "node_leafs_storage.h"

#include <boost/uuid/uuid_hash.hpp>

#include <cereal/types/vector.hpp>

#include <iterator>
#include <set>
#include <unordered_map>
#include <algorithm>
#include <utility>

NAMESPACE_BEGIN(blue_sky::tree)
namespace bs_detail = blue_sky::detail;

using existing_index = typename node::existing_index;

/// link erase options
enum class EraseOpts { Normal = 0, Silent = 1, DontResetOwner = 2 };

using bs_detail::shared;
using node_impl_mutex = std::shared_mutex;

static constexpr auto noop_postproc_f = [](const auto&) {};

/*-----------------------------------------------------------------------------
 *  node_impl
 *-----------------------------------------------------------------------------*/
class BS_HIDDEN_API node_impl : public bs_detail::sharded_same_mutex<node_impl_mutex, 2> {
public:
	friend class node;

	// lock granularity
	enum { Metadata, Links };

	// leafs
	links_container links_;
	// metadata
	std::weak_ptr<link> handle_;

	// timeout for most queries
	const caf::duration timeout;
	// scoped actor for requests
	caf::scoped_actor factor;

	// local node group
	caf::group self_grp;

	// append private behavior to public iface
	using actor_type = node::actor_type::extend<
		// terminate actor
		caf::reacts_to<a_bye>,
		// erase link by ID with specified options
		caf::replies_to<a_node_erase, lid_type, EraseOpts>::with<std::size_t>,
		// track link rename
		caf::reacts_to<a_ack, a_lnk_rename, lid_type, std::string, std::string>,
		// track link status
		caf::reacts_to<a_ack, a_lnk_status, lid_type, Req, ReqStatus, ReqStatus>,
		// ack on insert - reflect insert from sibling node actor
		caf::reacts_to<a_ack, a_node_insert, lid_type, size_t, InsertPolicy>,
		// ack on link move
		caf::reacts_to<a_ack, a_node_insert, lid_type, size_t, size_t>,
		// ack on link erase from sibling node
		caf::reacts_to<a_ack, a_node_erase, lids_v, std::vector<std::string>>
	>;

	static auto actor(const node& N) {
		return caf::actor_cast<actor_type>(N.actor_);
	}

	// make request to given link L
	template<typename R, typename... Args>
	auto actorf(const node& N, Args&&... args) {
		return blue_sky::actorf<R>(
			factor, caf::actor_cast<actor_type>(N.actor_), timeout, std::forward<Args>(args)...
		);
	}
	// same as above but with configurable timeout
	template<typename R, typename... Args>
	auto actorf(const node& N, timespan timeout, Args&&... args) {
		return blue_sky::actorf<R>(
			factor, caf::actor_cast<actor_type>(N.actor_), timeout, std::forward<Args>(args)...
		);
	}

	virtual auto spawn_actor(std::shared_ptr<node_impl> nimpl, const std::string& gid) const -> caf::actor;

	// default & copy ctor
	node_impl(node* super);
	node_impl(const node_impl&, node* super);
	node_impl(node_impl&&, node* super);

	///////////////////////////////////////////////////////////////////////////////
	//  iterate
	//
	template<Key K = Key::AnyOrder>
	auto begin() const {
		return links_.get<Key_tag<K>>().begin();
	}
	template<Key K = Key::AnyOrder>
	auto end() const {
		return links_.get<Key_tag<K>>().end();
	}

	template<Key K, Key R = Key::AnyOrder>
	auto project(iterator<K> pos) const {
		if constexpr(K == R)
			return pos;
		else
			return links_.project<Key_tag<R>>(std::move(pos));
	}

	// convert iterator to offset from beginning of AnyOrder index
	template<Key K = Key::AnyOrder>
	auto to_index(iterator<K> pos) const -> existing_index {
		auto& I = links_.get<Key_tag<Key::AnyOrder>>();
		if(auto ipos = project<K>(std::move(pos)); ipos != I.end())
			return static_cast<std::size_t>(std::distance(I.begin(), ipos));
		return {};
	}

	///////////////////////////////////////////////////////////////////////////////
	//  search
	//
	template<Key K, Key R = Key::AnyOrder>
	auto find(const Key_type<K>& key) const {
		if constexpr(K == Key::AnyOrder)
			// prevent indexing past array size
			return project<K, R>(std::next( begin<K>(), std::min(key, links_.size()) ));
		else
			return project<K, R>(links_.get<Key_tag<K>>().find(key));
	}

	// same as find, but returns link (null if not found)
	template<Key K>
	auto search(const Key_type<K>& key) const -> sp_link {
		if(auto p = find<K, K>(key); p != links_.get<Key_tag<K>>().end())
			return *p;
		return nullptr;
	}

	auto search(const std::string& key, Key key_meaning) const -> sp_link;

	// index of link with given key in AnyOrder index
	template<Key K>
	auto index(const Key_type<K>& key) const -> existing_index {
		return to_index(find<K>(key));
	}

	auto index(const std::string& key, Key key_meaning) const -> existing_index;

	// equal key
	template<Key K = Key::ID>
	auto equal_range(const Key_type<K>& key) const -> range<K> {
		if constexpr(K != Key::AnyOrder) {
			return links_.get<Key_tag<K>>().equal_range(key);
		}
		else {
			auto pos = find<K>(key);
			return {pos, std::next(pos)};
		}
	}

	auto equal_range(const std::string& key, Key key_meaning) const -> links_v;

	///////////////////////////////////////////////////////////////////////////////
	//  keys & values
	//
	template<Key K, typename Iterator>
	static auto keys(Iterator start, Iterator finish) {
		return range_t<Iterator>{ std::move(start), std::move(finish) }
		.template extract_keys<K>();
	}

	template<Key K, typename Container>
	static auto keys(const Container& links) {
		return keys<K>(links.begin(), links.end());
	}

	template<Key K>
	auto keys() const {
		return keys<K>(links_);
	}

	template<Key K>
	auto values() const -> links_v {
		auto res = links_v(links_.size());
		std::copy(begin<K>(), end<K>(), res.begin());
		return res;
	}

	auto leafs(Key order) const -> links_v;

	auto size() const -> std::size_t;

	///////////////////////////////////////////////////////////////////////////////
	//  insert
	//
	using leaf_postproc_fn = function_view< void(const sp_link&) >;

	auto insert(
		sp_link L, const InsertPolicy pol = InsertPolicy::AllowDupNames,
		leaf_postproc_fn ppf = noop_postproc_f
	) -> insert_status<Key::ID>;

	///////////////////////////////////////////////////////////////////////////////
	//  erase
	//
	template<Key K = Key::ID>
	auto erase(
		const Key_type<K>& key, leaf_postproc_fn ppf = noop_postproc_f,
		bool dont_reset_owner = false
	) -> size_t {
		if constexpr(K == Key::ID || K == Key::AnyOrder)
			return erase_impl(find<K, Key::ID>(key), std::move(ppf), dont_reset_owner);
		else
			return erase<K>(equal_range<K>(key), std::move(ppf), dont_reset_owner);
	}

	auto erase(const std::string& key, Key key_meaning, leaf_postproc_fn ppf = noop_postproc_f) -> size_t;

	auto erase(const lids_v& r, leaf_postproc_fn ppf = noop_postproc_f) -> std::size_t;

	///////////////////////////////////////////////////////////////////////////////
	//  rename
	//
	template<Key K = Key::ID>
	auto rename(
		const Key_type<K>& key, const std::string& new_name
	) -> size_t {
		// [NOTE] does rename via link, index update relies on retranslating rename ack message
		auto res = 0;
		if constexpr(K == Key::ID || K == Key::AnyOrder) {
			if(auto p = find<K, K>(key); p != end<K>()) {
				(*p)->rename(new_name);
				res = 1;
			}
		}
		else {
			for(auto& p : equal_range<K>(key)) {
				p->rename(new_name);
				++res;
			}
		}
		return res;
	}

	// update node indexes to match current link content
	auto refresh(const lid_type& lid) -> void;

	///////////////////////////////////////////////////////////////////////////////
	//  rearrange
	//
	template<Key K>
	auto rearrange(const std::vector<Key_type<K>>& new_order) {
		// convert vector of keys into vector of iterators
		std::vector<std::reference_wrapper<const sp_link>> i_order;
		i_order.reserve(new_order.size());
		for(const auto& k : new_order)
			i_order.push_back(std::ref(*find<K>(k)));
		// apply order
		links_.get<Key_tag<Key::AnyOrder>>().rearrange(i_order.begin());
	}

	///////////////////////////////////////////////////////////////////////////////
	//  misc
	//
	void set_handle(const sp_link& new_handle);

	auto propagate_owner(bool deep) -> void;

	// postprocessing of just inserted link
	// if link points to node, return it
	static sp_node adjust_inserted_link(const sp_link& lnk, const sp_node& target);

	// obtain pointer to owner node
	auto super() const -> sp_node;
	// get node's group ID
	auto gid() const -> std::string;

private:
	node* super_;

	// returns index of removed element
	// [NOTE] don't do range checking
	auto erase_impl(
		iterator<Key::ID> key, leaf_postproc_fn ppf = noop_postproc_f,
		bool dont_reset_owner = false
	) -> std::size_t;

	// erase multiple elements given in valid (!) range
	template<Key K = Key::ID>
	auto erase(
		const range<K>& r, leaf_postproc_fn ppf = noop_postproc_f,
		bool dont_reset_owner = false
	) -> size_t {
		size_t res = 0;
		for(auto x = r.begin(); x != r.end();) {
			erase_impl(project<K, Key::ID>(x++), ppf, dont_reset_owner);
			++res;
		}
		return res;
	}
};
using sp_nimpl = std::shared_ptr<node_impl>;

NAMESPACE_END(blue_sky::tree)

BS_ALLOW_ENUMOPS(blue_sky::tree::EraseOpts)
