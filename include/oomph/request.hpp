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

#include <oomph/util/pimpl.hpp>
#include <oomph/detail/request_state.hpp>

namespace oomph
{
class send_request
{
  private:
    using data_type = detail::request_state;
    using shared_request_ptr = detail::shared_request_ptr;
    friend class communicator;
    friend class communicator_impl;

    shared_request_ptr m_data;

    send_request(shared_request_ptr&& data) noexcept
    : m_data{std::move(data)}
    {
    }

  public:
    send_request() = default;
    send_request(send_request const&) = delete;
    send_request(send_request&&) = default;
    send_request& operator=(send_request const&) = delete;
    send_request& operator=(send_request&&) = default;

  public:
    bool is_ready() const noexcept
    {
        if (!m_data) return false;
        return m_data->m_ready;
    }
    bool test();
    void wait();
};

class recv_request
{
  private:
    using data_type = detail::request_state;
    using shared_request_ptr = detail::shared_request_ptr;
    friend class communicator;
    friend class communicator_impl;

    shared_request_ptr m_data;

    recv_request(shared_request_ptr&& data) noexcept
    : m_data{std::move(data)}
    {
    }

  public:
    recv_request() = default;
    recv_request(recv_request const&) = delete;
    recv_request(recv_request&&) = default;
    recv_request& operator=(recv_request const&) = delete;
    recv_request& operator=(recv_request&&) = default;

  public:
    bool is_ready() const noexcept
    {
        if (!m_data) return false;
        return m_data->m_ready;
    }
    bool test();
    void wait();
    bool cancel();
};

} // namespace oomph
