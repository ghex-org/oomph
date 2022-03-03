/*
 * oomph-org
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <oomph/barrier.hpp>
#include <bindings/fortran/oomph_defs.hpp>
#include <bindings/fortran/context_bind.hpp>
#include <mpi.h>
#include <sched.h>
#include <sys/sysinfo.h>

namespace
{
oomph::context* oomph_context;
#ifdef OOMPH_ENABLE_BARRIER
oomph::barrier* oomph_barrier_obj = nullptr;
#endif
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
#ifdef OOMPH_ENABLE_BARRIER
    oomph::barrier&
    barrier()
    {
        return *oomph_barrier_obj;
    }
#endif
    int nthreads = 1;
} // namespace fort
} // namespace oomph

extern "C" void
oomph_init(int nthreads, MPI_Fint fcomm)
{
    /* the fortran-side mpi communicator must be translated to C */
    MPI_Comm ccomm = MPI_Comm_f2c(fcomm);
    oomph_context = new oomph::context{ccomm, nthreads > 1};
    oomph::fort::nthreads = nthreads;
#ifdef OOMPH_ENABLE_BARRIER
    oomph_barrier_obj = new oomph::barrier(*oomph_context, nthreads);
#endif
}

extern "C" void
oomph_finalize()
{
    delete oomph_context;
#ifdef OOMPH_ENABLE_BARRIER
    delete oomph_barrier_obj;
#endif
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

#ifdef OOMPH_ENABLE_BARRIER
extern "C" void
oomph_barrier(int type)
{
    switch (type)
        {
        case (oomph::fort::OomphBarrierThread):
            oomph::fort::barrier().thread_barrier();
            break;
        case (oomph::fort::OomphBarrierRank):
            oomph::fort::barrier().rank_barrier();
            break;
        default:
            oomph::fort::barrier()();
            break;
        }
}
#endif
