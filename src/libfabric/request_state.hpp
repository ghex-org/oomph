/*
 * ghex-org
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <oomph/request.hpp>
#include "../request_state_base.hpp"
#include "./operation_context.hpp"

namespace oomph
{
namespace detail
{

struct request_state : public request_state_base<false>
{
    using base = request_state_base<false>;
    using operation_context = oomph::libfabric::operation_context;

    operation_context                      m_operation_context;
    util::unsafe_shared_ptr<request_state> m_self_ptr;

    request_state(oomph::context_impl* ctxt, oomph::communicator_impl* comm, std::size_t* scheduled,
        rank_type rank, tag_type tag, cb_type&& cb)
    : base{ctxt, comm, scheduled, rank, tag, std::move(cb)}
    , m_operation_context{this}
    {
        //[[maybe_unused]] auto scp = oomph::libfabric::ctx_deb.scope(NS_DEBUG::ptr(this), __func__);
    }

    //~request_state()
    //{
    //    [[maybe_unused]] auto scp = oomph::libfabric::ctx_deb.scope(NS_DEBUG::ptr(this), __func__);
    //}

    void progress();

    bool cancel();
};

struct shared_request_state : public request_state_base<true>
{
    using base = request_state_base<true>;
    using operation_context = oomph::libfabric::operation_context;

    operation_context                     m_operation_context;
    std::shared_ptr<shared_request_state> m_self_ptr;

    shared_request_state(oomph::context_impl* ctxt, oomph::communicator_impl* comm,
        std::atomic<std::size_t>* scheduled, rank_type rank, tag_type tag, cb_type&& cb)
    : base{ctxt, comm, scheduled, rank, tag, std::move(cb)}
    , m_operation_context{this}
    {
    }

    void progress();

    bool cancel();
};

} // namespace detail
} // namespace oomph
