/*
 * ghex-org
 *
 * Copyright (c) 2014-2023, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <hwmalloc/numa.hpp>
#include <oomph/config.hpp>

// paths relative to backend
#include <context.hpp>
#include <communicator.hpp>
#include <../message_buffer.hpp>
#include <../util/heap_pimpl_src.hpp>

OOMPH_INSTANTIATE_HEAP_PIMPL(oomph::context_impl)

namespace oomph
{

bool
send_request::is_ready() const noexcept
{
    if (!m) return true;
    return m->is_ready();
}

bool
send_request::test()
{
    if (!m || m->is_ready()) return true;
    m->progress();
    return m->is_ready();
}

void
send_request::wait()
{
    if (!m) return;
    while (!m->is_ready()) m->progress();
}

bool
recv_request::is_ready() const noexcept
{
    if (!m) return true;
    return m->is_ready();
}

bool
recv_request::is_canceled() const noexcept
{
    if (!m) return true;
    return m->is_canceled();
}

bool
recv_request::test()
{
    if (!m || m->is_ready()) return true;
    m->progress();
    return m->is_ready();
}

void
recv_request::wait()
{
    if (!m) return;
    while (!m->is_ready()) m->progress();
}

bool
recv_request::cancel()
{
    if (!m) return false;
    if (m->is_ready()) return false;
    return m->cancel();
}

bool
shared_recv_request::is_ready() const noexcept
{
    if (!m) return true;
    return m->is_ready();
}

bool
shared_recv_request::is_canceled() const noexcept
{
    if (!m) return true;
    return m->is_canceled();
}

bool
shared_recv_request::test()
{
    if (!m || m->is_ready()) return true;
    m->progress();
    return m->is_ready();
}

void
shared_recv_request::wait()
{
    if (!m) return;
    while (!m->is_ready()) m->progress();
}

bool
shared_recv_request::cancel()
{
    if (!m) return false;
    if (m->is_ready()) return false;
    return m->cancel();
}

bool
send_multi_request::is_ready() const noexcept
{
    if (!m) return true;
    return (m->m_counter == 0);
}

bool
send_multi_request::test()
{
    if (!m) return true;
    if (m->m_counter == 0) return true;
    m->m_comm->progress();
    return (m->m_counter == 0);
}

void
send_multi_request::wait()
{
    if (!m) return;
    if (m->m_counter == 0) return;
    while (m->m_counter > 0) m->m_comm->progress();
}

void
detail::request_state::progress()
{
    m_comm->progress();
}

bool
detail::request_state::cancel()
{
    return m_comm->cancel_recv(this);
}

void
detail::shared_request_state::progress()
{
    m_ctxt->progress();
}

bool
detail::shared_request_state::cancel()
{
    return m_ctxt->cancel_recv(this);
}

} // namespace oomph
