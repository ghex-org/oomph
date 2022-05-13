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

namespace oomph
{
namespace detail
{
struct request_state : public request_state_base<false>
{
    using base = request_state_base<false>;
    void*                                  m_ucx_ptr;
    ucx_mutex&                             m_mutex;
    util::unsafe_shared_ptr<request_state> m_self_ptr;

    request_state(oomph::context_impl* ctxt, oomph::communicator_impl* comm, std::size_t* scheduled,
        rank_type rank, tag_type tag, cb_type&& cb, void* ucx_ptr, ucx_mutex& mtx)
    : base{ctxt, comm, scheduled, rank, tag, std::move(cb)}
    , m_ucx_ptr{ucx_ptr}
    , m_mutex{mtx}
    {
    }

    void progress();

    bool cancel();
};

struct shared_request_state : public request_state_base<true>
{
    using base = request_state_base<true>;
    void*                                 m_ucx_ptr;
    ucx_mutex&                             m_mutex;
    std::shared_ptr<shared_request_state> m_self_ptr;

    shared_request_state(oomph::context_impl* ctxt, oomph::communicator_impl* comm,
        std::atomic<std::size_t>* scheduled, rank_type rank, tag_type tag, cb_type&& cb,
        void* ucx_ptr, ucx_mutex& mtx)
    : base{ctxt, comm, scheduled, rank, tag, std::move(cb)}
    , m_ucx_ptr{ucx_ptr}
    , m_mutex{mtx}
    {
    }

    void progress();

    bool cancel();
};

} // namespace detail
} // namespace oomph
