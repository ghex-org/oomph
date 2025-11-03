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

#include <oomph/util/mpi_error.hpp>
#include <oomph/util/moved_bit.hpp>

#include <nccl_error.hpp>
#include <../mpi_comm.hpp>

#include <nccl.h>

namespace oomph::detail
{
class nccl_comm
{
    ncclComm_t             m_comm;
    oomph::util::moved_bit m_moved;

  public:
    nccl_communicator(mpi_comm mpi_comm)
    {
        ncclUniqueId id;
        if (mpi_comm.rank() == 0) { OOMPH_CHECK_NCCL_RESULT(ncclGetUniqueId(&id)); }

        OOMPH_CHECK_MPI_RESULT(MPI_Bcast(&id, sizeof(id), MPI_BYTE, 0, mpi_comm.get()));

        OOMPH_CHECK_NCCL_RESULT(ncclCommInitRank(&m_comm, mpi_comm.size(), id, mpi_comm.rank()));
        ncclResult_t result;
        do {
            OOMPH_CHECK_NCCL_RESULT(ncclCommGetAsyncError(m_comm, &result));
        }
    }
    nccl_comm(nccl_comm&&) noexcept = default;
    nccl_comm& operator=(nccl_comm&&) noexcept = default;
    nccl_comm(nccl_comm const&) = delete;
    nccl_comm& operator=(nccl_comm const&) = delete;
    ~nccl_comm() noexcept
    {
        if (!m_moved)
        {
            // TODO
            // OOMPH_CHECK_CUDA_RESULT_NOEXCEPT(cudaDeviceSynchronize());
            cudaDeviceSynchronize();
            OOMPH_CHECK_NCCL_RESULT_NOEXCEPT(ncclCommDestroy(m_comm));
        }
    }
};
} // namespace oomph::detail
