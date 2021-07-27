/*
 * GridTools
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <oomph/util/mpi_error.hpp>

struct mpi_test_fixture : public ::testing::Test
{
    static void SetUpTestSuite() {}

    static void TearDownTestSuite() {}

    void SetUp() override
    {
        OOMPH_CHECK_MPI_RESULT(MPI_Comm_rank(MPI_COMM_WORLD, &world_rank));
        OOMPH_CHECK_MPI_RESULT(MPI_Comm_size(MPI_COMM_WORLD, &world_size));
    }

    //void TearDown() {}

  protected:
    int world_rank;
    int world_size;
};
