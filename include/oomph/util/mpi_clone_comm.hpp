#pragma once

#include <oomph/util/mpi_error.hpp>

namespace oomph
{
namespace util
{
inline MPI_Comm
mpi_clone_comm(MPI_Comm mpi_comm)
{
    MPI_Comm new_comm;
    OOMPH_CHECK_MPI_RESULT(MPI_Comm_dup(mpi_comm, &new_comm));
    return new_comm;
}

} // namespace util
} // namespace oomph
