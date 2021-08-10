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

#include <mpi.h>
#include <iostream>

namespace oomph
{
struct mpi_environment
{
    int size;

    mpi_environment(bool thread_safe, int& argc, char**& argv)
    {
        int mode;
        if (thread_safe)
        {
            MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &mode);
            if (mode != MPI_THREAD_MULTIPLE)
            {
                std::cerr << "MPI_THREAD_MULTIPLE not supported by MPI, aborting\n";
                std::terminate();
            }
        }
        else
        {
            MPI_Init_thread(&argc, &argv, MPI_THREAD_SINGLE, &mode);
        }
        MPI_Comm_size(MPI_COMM_WORLD, &size);
    }

    mpi_environment(mpi_environment const&) = delete;

    ~mpi_environment()
    {
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Finalize();
    }
};

} // namespace oomph
