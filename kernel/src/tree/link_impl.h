/// @file
/// @author uentity
/// @date 22.07.2019
/// @brief Link implementation that contain link state
/// @copyright
/// This Source Code Form is subject to the terms of the Mozilla Public License,
/// v. 2.0. If a copy of the MPL was not distributed with this file,
/// You can obtain one at https://mozilla.org/MPL/2.0/
#pragma once

#include <bs/tree/link.h>
#include <bs/actor_common.h>
#include <bs/kernel/radio.h>
#include <bs/detail/enumops.h>
#include <bs/detail/function_view.h>
#include <bs/detail/sharded_mutex.h>

#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#if defined(_MSC_VER)
#include <bs/detail/spinlock.h>
#endif

NAMESPACE_BEGIN(blue_sky::tree)
namespace bs_detail = blue_sky::detail;

inline const auto nil_uid = boost::uuids::nil_uuid();
inline const std::string nil_oid = to_string(nil_uid);

using link_impl_mutex = std::mutex;

class BS_HIDDEN_API link_impl : public bs_detail::sharded_mutex<link_impl_mutex> {
public:
	using mutex_t = bs_detail::sharded_mutex<link_impl_mutex>;

	lid_type id_;
	std::string name_;
	Flags flags_;

	// timeout for most queries
	const caf::duration timeout;
	// scoped actor for requests
	caf::scoped_actor factor;

	// keep local link group
	caf::group self_grp;

	/// owner node
	std::weak_ptr<tree::node> owner_;

	/// status of operations
	struct status_handle {
		ReqStatus value = ReqStatus::Void;

#ifdef _MSC_VER
		mutable bs_detail::spinlock guard;
#else
		mutable std::mutex guard;
#endif
	};
	status_handle status_[2];

	link_impl();
	link_impl(std::string name, Flags f);
	virtual ~link_impl();

	///////////////////////////////////////////////////////////////////////////////
	//  API
	//
	// make request to given link L
	template<typename R, typename Link, typename... Args>
	auto actorf(const Link& L, Args&&... args) {
		return blue_sky::actorf<R>(factor, Link::actor(L), timeout, std::forward<Args>(args)...);
	}
	// same as above but with configurable timeout
	template<typename R, typename Link, typename... Args>
	auto actorf(const Link& L, timespan timeout, Args&&... args) {
		return blue_sky::actorf<R>(factor, Link::actor(L), timeout, std::forward<Args>(args)...);
	}

	// spawn actor corresponding to this impl type
	virtual auto spawn_actor(std::shared_ptr<link_impl> limpl) const -> caf::actor;

	/// obtain inode pointer
	/// default impl do it via `data_ex()` call
	virtual auto get_inode() -> result_or_err<inodeptr>;

	/// [NOTE] download pointee data - must be implemented by derived links
	virtual auto data() -> result_or_err<sp_obj> = 0;

	auto req_status(Req request) const -> ReqStatus;

	using on_rs_changed_fn = function_view< void(Req, ReqStatus /*new*/, ReqStatus /*old*/) >;
	static constexpr auto on_rs_changed_noop = [](Req, ReqStatus, ReqStatus) {};

	auto rs_reset(
		Req request, ReqReset cond, ReqStatus new_rs, ReqStatus old_rs = ReqStatus::Void,
		on_rs_changed_fn on_rs_changed = on_rs_changed_noop
	) -> ReqStatus;

	auto reset_owner(const sp_node& new_owner) -> void;

	/// create or set or create inode for given target object
	/// [NOTE] if `new_info` is non-null, returned inode may be NOT EQUAL to `new_info`
	static auto make_inode(const sp_obj& target, inodeptr new_info = nullptr) -> inodeptr;
};

using sp_limpl = std::shared_ptr<link_impl>;

///////////////////////////////////////////////////////////////////////////////
//  derived links
//
struct BS_HIDDEN_API ilink_impl : link_impl {
	inodeptr inode_;

	using super = link_impl;

	ilink_impl();
	ilink_impl(std::string name, const sp_obj& data, Flags f);

	// returns stored pointer
	auto get_inode() -> result_or_err<inodeptr> override final;
};

struct BS_HIDDEN_API hard_link_impl : ilink_impl {
	sp_obj data_;

	using super = ilink_impl;

	hard_link_impl();
	hard_link_impl(std::string name, sp_obj data, Flags f);

	auto spawn_actor(sp_limpl limpl) const -> caf::actor override;

	auto data() -> result_or_err<sp_obj> override;
	auto set_data(sp_obj obj) -> void;
};

struct BS_HIDDEN_API weak_link_impl : ilink_impl {
	std::weak_ptr<objbase> data_;

	using super = ilink_impl;

	weak_link_impl();
	weak_link_impl(std::string name, const sp_obj& data, Flags f);

	auto spawn_actor(sp_limpl limpl) const -> caf::actor override;

	auto data() -> result_or_err<sp_obj> override;
	auto set_data(const sp_obj& obj) -> void;
};

struct BS_HIDDEN_API sym_link_impl : link_impl {
	std::string path_;

	using super = link_impl;

	sym_link_impl();
	sym_link_impl(std::string name, std::string path, Flags f);

	auto spawn_actor(sp_limpl limpl) const -> caf::actor override;

	auto data() -> result_or_err<sp_obj> override;

	auto pointee() const -> result_or_err<sp_link>;
};

auto to_string(Req) -> const char*;
auto to_string(ReqStatus) -> const char*;

NAMESPACE_END(blue_sky::tree)

BS_ALLOW_ENUMOPS(blue_sky::tree::ReqReset)
