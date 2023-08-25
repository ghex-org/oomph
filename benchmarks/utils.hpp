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

#ifdef OOMPH_BENCHMARKS_MT
#define THREADID omp_get_thread_num()
#else
#define THREADID 0
#endif

namespace oomph {

inline int
exit(char const* executable)
{
    std::cerr << "Usage: " << executable << " [niter] [msg_size] [inflight]" << std::endl;
    std::cerr << "       run with 2 MPI processes: e.g.: mpirun -np 2 ..." << std::endl;
    return 1;
}

} // namespace oomph
