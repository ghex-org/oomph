/* 
 * GridTools
 * 
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 * 
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 */
#pragma once

#include <oomph/context.hpp>
#include "./mpi_comm.hpp"
#include "./unique_ptr_set.hpp"
#include "./rank_topology.hpp"
#include <iostream>

namespace oomph
{
class context_base
{
  public:
    using rank_type = communicator::rank_type;

  protected:
    mpi_comm                          m_mpi_comm;
    bool const                        m_thread_safe;
    rank_topology const               m_rank_topology;
    unique_ptr_set<communicator_impl> m_comms_set;

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
        else if (!m_thread_safe && !(mpi_thread_safety == MPI_THREAD_SINGLE))
            std::cerr << "oomph warning: MPI thread safety is too restrictive" << std::endl;
    }

  public:
    rank_type            rank() const noexcept { return m_mpi_comm.rank(); }
    rank_type            size() const noexcept { return m_mpi_comm.size(); }
    rank_topology const& topology() const noexcept { return m_rank_topology; }
    MPI_Comm             get_comm() const noexcept { return m_mpi_comm; }

    void deregister_communicator(communicator_impl* c) { m_comms_set.remove(c); }
};

} // namespace oomph
