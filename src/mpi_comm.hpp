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

#include <oomph/communicator.hpp>
#include <oomph/util/mpi_error.hpp>

namespace oomph
{
class mpi_comm
{
  public:
    using rank_type = communicator::rank_type;

  private:
    MPI_Comm  m_comm;
    rank_type m_rank;
    rank_type m_size;

  public:
    mpi_comm(MPI_Comm comm)
    : m_comm{comm}
    , m_rank{[](MPI_Comm c) {
        int r;
        OOMPH_CHECK_MPI_RESULT(MPI_Comm_rank(c, &r));
        return r;
    }(comm)}
    , m_size{[](MPI_Comm c) {
        int s;
        OOMPH_CHECK_MPI_RESULT(MPI_Comm_size(c, &s));
        return s;
    }(comm)}
    {
    }

    mpi_comm(mpi_comm const&) = default;
    mpi_comm& operator=(mpi_comm const&) = default;

    rank_type rank() const noexcept { return m_rank; }
    rank_type size() const noexcept { return m_size; }

             operator MPI_Comm() const noexcept { return m_comm; }
    MPI_Comm get() const noexcept { return m_comm; }
};

} // namespace oomph
