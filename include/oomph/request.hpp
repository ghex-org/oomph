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

#include <oomph/util/pimpl.hpp>
#include <oomph/detail/request_state.hpp>
#include <vector>

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
        if (!m_data) return true;
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
        if (!m_data) return true;
        return m_data->m_ready;
    }
    bool test();
    void wait();
    bool cancel();
};

namespace detail
{
struct request_state2;
struct shared_request_state;
} // namespace detail

class send_request2
{
  protected:
    using state_type = detail::request_state2;
    friend class communicator;
    friend class communicator_impl;

    std::shared_ptr<state_type> m;

    send_request2(std::shared_ptr<state_type> s) noexcept
    : m{std::move(s)}
    {
    }

  public:
    send_request2() = default;
    send_request2(send_request2 const&) = delete;
    send_request2(send_request2&&) = default;
    send_request2& operator=(send_request2 const&) = delete;
    send_request2& operator=(send_request2&&) = default;

  public:
    bool is_ready() const noexcept;
    bool is_canceled() const noexcept;
    bool test();
    void wait();
};

class recv_request2
{
  protected:
    using state_type = detail::request_state2;
    friend class communicator;
    friend class communicator_impl;

    std::shared_ptr<state_type> m;

    recv_request2(std::shared_ptr<state_type> s) noexcept
    : m{std::move(s)}
    {
    }

  public:
    recv_request2() = default;
    recv_request2(recv_request2 const&) = delete;
    recv_request2(recv_request2&&) = default;
    recv_request2& operator=(recv_request2 const&) = delete;
    recv_request2& operator=(recv_request2&&) = default;

  public:
    bool is_ready() const noexcept;
    bool is_canceled() const noexcept;
    bool test();
    void wait();
    bool cancel();
};

class shared_recv_request
{
  private:
    using state_type = detail::shared_request_state;
    friend class communicator;
    friend class communicator_impl;
  private:

    std::shared_ptr<state_type> m;

    shared_recv_request(std::shared_ptr<state_type> s) noexcept
    : m{std::move(s)}
    {
    }

  public:
    shared_recv_request() = default;
    shared_recv_request(shared_recv_request const&) = default;
    shared_recv_request(shared_recv_request&&) = default;
    shared_recv_request& operator=(shared_recv_request const&) = default;
    shared_recv_request& operator=(shared_recv_request&&) = default;

  public:
    bool is_ready() const noexcept;
    bool is_canceled() const noexcept;
    bool test();
    void wait();
    bool cancel();
};

} // namespace oomph
