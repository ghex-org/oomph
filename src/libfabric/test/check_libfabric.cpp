/*
 * ghex-org
 *
 * Copyright (c) 2014-2023, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <mpi.h>
#include <oomph/context.hpp>
#include "../benchmarks/mpi_environment.hpp"
//
#include "../communicator.hpp"
#include "../context.hpp"

int main(int argc, char** argv)
{
    using namespace oomph;
    bool const message_pool_never_free = false;
    std::size_t const message_pool_reserve = 1024 * 1024 * 128;
    bool const multi_threaded = true;
    bool debug = true;
    //
    mpi_environment env(multi_threaded, argc, argv);
    auto ctxt =
        context_impl(MPI_COMM_WORLD, true, message_pool_never_free, message_pool_reserve, debug);
}
