#pragma once

#include <oomph/request.hpp>
#include <oomph/util/mpi_error.hpp>

namespace oomph
{
class request::impl
{
  public:
    MPI_Request m_req;
    bool        m_is_ready;

    impl()
    : m_is_ready(true)
    {
    }

    impl(MPI_Request req)
    : m_req{req}
    , m_is_ready(false)
    {
    }

    ~impl();

    bool is_ready()
    {
        if (m_is_ready) return true;
        int flag;
        OOMPH_CHECK_MPI_RESULT(MPI_Test(&m_req, &flag, MPI_STATUS_IGNORE));
        if (flag) m_is_ready = true;
        return m_is_ready;
    }

    void wait()
    {
        if (m_is_ready) return;
        OOMPH_CHECK_MPI_RESULT(MPI_Wait(&m_req, MPI_STATUS_IGNORE));
    }
};

//template<>
//request::request<MPI_Request>(MPI_Request r);

} // namespace oomph
