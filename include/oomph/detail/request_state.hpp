/*
 * ghex-org
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <memory>
#include <boost/pool/pool.hpp>

namespace oomph
{
class communicator;
class communicator_impl;

namespace detail
{
struct request_state
{
    // reserved space may be used by transport layers for special storage
    using reserved = std::array<uint64_t, OOMPH_TRANSPORT_RESERVED_SPACE>;
    reserved                m_reserved;
    communicator_impl*      m_comm;
    std::size_t*            m_scheduled;
    bool                    m_ready = false;
    std::size_t             m_ref_count = 0;

    request_state(communicator_impl* comm, std::size_t* scheduled) noexcept
    : m_comm{comm}
    , m_scheduled{scheduled}
    {
    }
};

class shared_request_ptr
{
  private:
    request_state* m_ptr = nullptr;
    boost::pool<>* m_pool = nullptr;

  public:
    shared_request_ptr(boost::pool<>* pool, communicator_impl* comm, std::size_t* scheduled)
    : m_pool{pool}
    {
        m_ptr = new (m_pool->malloc()) request_state(comm, scheduled);
        m_ptr->m_ref_count = 1;
    }

    shared_request_ptr() = default;

    shared_request_ptr(shared_request_ptr&& other) noexcept
    : m_ptr{std::exchange(other.m_ptr, nullptr)}
    , m_pool{other.m_pool}
    {
    }

    shared_request_ptr& operator=(shared_request_ptr&& other) noexcept
    {
        destroy();
        m_ptr = std::exchange(other.m_ptr, nullptr);
        m_pool = other.m_pool;
        return *this;
    }

    shared_request_ptr(shared_request_ptr const& other) noexcept
    : m_ptr{other.m_ptr}
    , m_pool{other.m_pool}
    {
        if (m_ptr) ++m_ptr->m_ref_count;
    }

    shared_request_ptr& operator=(shared_request_ptr const& other) noexcept
    {
        destroy();
        m_ptr = other.m_ptr;
        m_pool = other.m_pool;
        if (m_ptr) ++m_ptr->m_ref_count;
        return *this;
    }

    ~shared_request_ptr() { destroy(); }

    operator bool() const noexcept { return m_ptr; }

    request_state* get() const noexcept { return m_ptr; }

    request_state* operator->() const noexcept { return m_ptr; }

    request_state& operator*() const noexcept { return *m_ptr; }

    void reset()
    {
        destroy();
        m_ptr = nullptr;
    }

  private:
    void destroy()
    {
        if (m_ptr)
        {
            if (--m_ptr->m_ref_count == 0)
            {
                m_ptr->~request_state();
                m_pool->free(m_ptr);
            }
        }
    }
};

} // namespace detail
} // namespace oomph
