/*
 * ghex-org
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <fortran/context_bind.hpp>
#include <mpi.h>
#include <sched.h>
#include <sys/sysinfo.h>

namespace
{
oomph::context* oomph_context;
}

namespace oomph
{
namespace fort
{
context&
get_context()
{
    return *oomph_context;
}
} // namespace fort
} // namespace oomph

extern "C" void
oomph_init(MPI_Fint fcomm)
{
    /* the fortran-side mpi communicator must be translated to C */
    MPI_Comm ccomm = MPI_Comm_f2c(fcomm);
    int      mpi_thread_safety;
    MPI_Query_thread(&mpi_thread_safety);
    const bool thread_safe = (mpi_thread_safety == MPI_THREAD_MULTIPLE);
    oomph_context = new oomph::context{ccomm, thread_safe};
}

extern "C" void
oomph_finalize()
{
    delete oomph_context;
}

extern "C" int
oomph_get_current_cpu()
{
    return sched_getcpu();
}

extern "C" int
oomph_get_ncpus()
{
    return get_nprocs_conf();
}
