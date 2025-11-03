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
} // namespace oomph
