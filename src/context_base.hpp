/*
 * ghex-org
 *
 * Copyright (c) 2014-2023, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <iostream>
#include <atomic>
#include <oomph/context.hpp>

// paths relative to backend
#include <../mpi_comm.hpp>
#include <../unique_ptr_set.hpp>
#include <../rank_topology.hpp>
#include <../increment_guard.hpp>

namespace oomph
{
class context_base
{
  public:
    using recursion_increment = increment_guard<std::atomic<std::size_t>>;

  protected:
    mpi_comm                          m_mpi_comm;
    bool const                        m_thread_safe;
    rank_topology const               m_rank_topology;
    unique_ptr_set<communicator_impl> m_comms_set;
    std::atomic<std::size_t>          m_recursion_depth = 0u;

  public:
    context_base(MPI_Comm comm, bool thread_safe)
    : m_mpi_comm{comm}
    , m_thread_safe{thread_safe}
    , m_rank_topology(comm)
    {
        int mpi_thread_safety;
        OOMPH_CHECK_MPI_RESULT(MPI_Query_thread(&mpi_thread_safety));
        if (m_thread_safe && !(mpi_thread_safety == MPI_THREAD_MULTIPLE))
            throw std::runtime_error("oomph: MPI is not thread safe!");
        else if (!m_thread_safe && !(mpi_thread_safety == MPI_THREAD_SINGLE) && rank() == 0)
            std::cerr << "oomph warning: MPI thread safety is higher than required" << std::endl;
    }

  public:
    rank_type            rank() const noexcept { return m_mpi_comm.rank(); }
    rank_type            size() const noexcept { return m_mpi_comm.size(); }
    rank_topology const& topology() const noexcept { return m_rank_topology; }
    MPI_Comm             get_comm() const noexcept { return m_mpi_comm; }
    bool                 thread_safe() const noexcept { return m_thread_safe; }

    void deregister_communicator(communicator_impl* c) { m_comms_set.remove(c); }

    bool has_reached_recursion_depth() const noexcept
    {
        return m_recursion_depth > OOMPH_RECURSION_DEPTH;
    }

    recursion_increment recursion() noexcept { return {m_recursion_depth}; }
};

} // namespace oomph
