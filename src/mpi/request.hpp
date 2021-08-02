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

#include <oomph/request.hpp>
#include <oomph/util/mpi_error.hpp>

namespace oomph
{
                
class request::impl
{
  public:
    MPI_Request m_req;
    bool        m_is_ready;
    bool        m_send_req = true;

    impl()
    : m_is_ready(true)
    {
    }

    impl(MPI_Request req, bool send_req = true)
    : m_req{req}
    , m_is_ready(false)
    , m_send_req(send_req)
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

    bool cancel()
    {
        if (m_send_req) return false;
        OOMPH_CHECK_MPI_RESULT(MPI_Cancel(&m_req));
        MPI_Status st;
        OOMPH_CHECK_MPI_RESULT(MPI_Wait(&m_req, &st));
        int flag = false;
        OOMPH_CHECK_MPI_RESULT(MPI_Test_cancelled(&st, &flag));
        return flag;
    }

    bool is_recv() const noexcept
    {
        return !(m_send_req);
    }
};

//template<>
//request::request<MPI_Request>(MPI_Request r);

} // namespace oomph
