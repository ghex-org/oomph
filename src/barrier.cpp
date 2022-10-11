/*
 * ghex-org
 *
 * Copyright (c) 2014-2022, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <hwmalloc/numa.hpp>
#include <oomph/config.hpp>
#include <oomph/barrier.hpp>

#if OOMPH_ENABLE_BARRIER
// paths relative to backend
#include <../communicator_set.hpp>

namespace oomph
{

barrier::barrier(context const& c, size_t n_threads)
: m_threads{n_threads}
, b_count2{m_threads}
, m_mpi_comm{c.mpi_comm()}
, m_context{c.m.get()}
{
}

void
barrier::operator()() const
{
    if (in_node1()) rank_barrier();
    else
        while (b_count2 == m_threads) communicator_set::get().progress(m_context);
    in_node2();
}

void
barrier::rank_barrier() const
{
    MPI_Request req = MPI_REQUEST_NULL;
    int         flag;
    MPI_Ibarrier(m_mpi_comm, &req);
    while (true)
    {
        communicator_set::get().progress(m_context);
        MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
        if (flag) break;
    }
}

bool
barrier::in_node1() const
{
    size_t expected = b_count;
    while (!b_count.compare_exchange_weak(expected, expected + 1, std::memory_order_relaxed))
        expected = b_count;
    if (expected == m_threads - 1)
    {
        b_count.store(0);
        return true;
    }
    else
    {
        while (b_count != 0) communicator_set::get().progress(m_context);
        return false;
    }
}

void
barrier::in_node2() const
{
    size_t ex = b_count2;
    while (!b_count2.compare_exchange_weak(ex, ex - 1, std::memory_order_relaxed)) ex = b_count2;
    if (ex == 1) { b_count2.store(m_threads); }
    else
    {
        while (b_count2 != m_threads) communicator_set::get().progress(m_context);
    }
}
} // namespace oomph

#endif
