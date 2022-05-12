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
#include <oomph/util/mpi_error.hpp>
#include "../request_state.hpp"

namespace oomph
{
struct mpi_request
{
    MPI_Request m_req;

    bool is_ready()
    {
        int flag;
        OOMPH_CHECK_MPI_RESULT(MPI_Test(&m_req, &flag, MPI_STATUS_IGNORE));
        return flag;
    }

    bool cancel()
    {
        OOMPH_CHECK_MPI_RESULT(MPI_Cancel(&m_req));
        MPI_Status st;
        OOMPH_CHECK_MPI_RESULT(MPI_Wait(&m_req, &st));
        int flag = false;
        OOMPH_CHECK_MPI_RESULT(MPI_Test_cancelled(&st, &flag));
        return flag;
    }
};

namespace detail
{
struct request_state2 : public request_state_base<false>
{
    using base = request_state_base<false>;
    mpi_request                             m_req;
    util::unsafe_shared_ptr<request_state2> m_self_ptr;
    std::size_t                             m_index;

    request_state2(oomph::context_impl* ctxt, oomph::communicator_impl* comm,
        std::size_t* scheduled, rank_type rank, tag_type tag, cb_type&& cb, mpi_request m)
    : base{ctxt, comm, scheduled, rank, tag, std::move(cb)}
    , m_req{m}
    {
    }

    void progress() {}

    bool cancel() { return true; }
};

struct shared_request_state : public request_state_base<true>
{
    using base = request_state_base<true>;
    mpi_request                           m_req;
    std::shared_ptr<shared_request_state> m_self_ptr;

    shared_request_state(oomph::context_impl* ctxt, oomph::communicator_impl* comm,
        std::atomic<std::size_t>* scheduled, rank_type rank, tag_type tag, cb_type&& cb,
        mpi_request m)
    : base{ctxt, comm, scheduled, rank, tag, std::move(cb)}
    , m_req{m}
    {
    }

    void progress();

    bool cancel() { return true; }
};

} // namespace detail
} // namespace oomph
