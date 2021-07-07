#pragma once

#include <oomph/request.hpp>
#include <oomph/util/mpi_error.hpp>

namespace oomph
{
class request::impl
{
  public:
    MPI_Request m_req;

    ~impl();

    bool is_ready()
    {
        int flag;
        OOMPH_CHECK_MPI_RESULT(MPI_Test(&m_req, &flag, MPI_STATUS_IGNORE));
        return flag;
    }

    void wait() { OOMPH_CHECK_MPI_RESULT(MPI_Wait(&m_req, MPI_STATUS_IGNORE)); }
};

template<>
request::request<MPI_Request>(MPI_Request r);

} // namespace oomph
