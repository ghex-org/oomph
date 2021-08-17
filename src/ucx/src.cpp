/*
 * GridTools
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "./context.hpp"
#include "./communicator.hpp"

namespace oomph
{
communicator_impl*
context_impl::get_communicator()
{
    auto send_worker = std::make_unique<worker_type>(
        get(), m_db, (m_thread_safe ? UCS_THREAD_MODE_SERIALIZED : UCS_THREAD_MODE_SINGLE));
    auto send_worker_ptr = send_worker.get();
    if (m_thread_safe)
    {
        ucx_lock l(m_mutex);
        m_workers.push_back(std::move(send_worker));
    }
    else
    {
        m_workers.push_back(std::move(send_worker));
    }
    auto comm =
        new communicator_impl{this, m_thread_safe, m_worker.get(), send_worker_ptr, m_mutex};
    m_comms_set.insert(comm);
    return comm;
}

context_impl::~context_impl()
{
    // ucp_worker_destroy should be called after a barrier
    // use MPI IBarrier and progress all workers
    MPI_Request req = MPI_REQUEST_NULL;
    int         flag;
    MPI_Ibarrier(m_mpi_comm, &req);
    while (true)
    {
        // make communicators from workers and progress
        for (auto& w_ptr : m_workers)
            communicator_impl{this, m_thread_safe, m_worker.get(), w_ptr.get(), m_mutex}.progress();
        communicator_impl{this, m_thread_safe, m_worker.get(), m_worker.get(), m_mutex}.progress();
        MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
        if (flag) break;
    }
    // close endpoints
    for (auto& w_ptr : m_workers) w_ptr->m_endpoint_cache.clear();
    m_worker->m_endpoint_cache.clear();
    // another MPI barrier to be sure
    MPI_Barrier(m_mpi_comm);
}

} // namespace oomph

#include "../src.cpp"
