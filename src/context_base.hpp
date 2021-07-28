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

namespace oomph
{
class context_base
{
  public:
    using rank_type = communicator::rank_type;

  protected:
    mpi_comm                           m_mpi_comm;
    rank_topology const                m_rank_topology;
    unique_ptr_set<communicator::impl> m_comms_set;

  public:
    context_base(MPI_Comm comm)
    : m_mpi_comm{comm}
    , m_rank_topology(comm)
    {
    }

  public:
    rank_type            rank() const noexcept { return m_mpi_comm.rank(); }
    rank_type            size() const noexcept { return m_mpi_comm.size(); }
    rank_topology const& topology() const noexcept { return m_rank_topology; }
    MPI_Comm get_comm() const noexcept { return m_mpi_comm; }

    void deregister_communicator(communicator::impl* c) { m_comms_set.remove(c); }
};

} // namespace oomph
