#pragma once

#include <oomph/util/moved_bit.hpp>
#include <oomph/util/mpi_clone_comm.hpp>

namespace oomph
{
namespace util
{
class mpi_comm_holder
{
  private:
    MPI_Comm  m;
    moved_bit m_moved;

  public:
    mpi_comm_holder(MPI_Comm comm)
    : m{util::mpi_clone_comm(comm)}
    {
    }
    mpi_comm_holder(mpi_comm_holder const&) = delete;
    mpi_comm_holder(mpi_comm_holder&&) noexcept = default;
    mpi_comm_holder& operator=(mpi_comm_holder const&) = delete;
    mpi_comm_holder& operator=(mpi_comm_holder&&) noexcept = default;
    ~mpi_comm_holder() noexcept
    {
        if (!m_moved) MPI_Comm_free(&m);
    }
    MPI_Comm get() const noexcept { return m; }
};
} // namespace util
} // namespace oomph
