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
#include <vector>
#include <oomph/detail/message_buffer.hpp>
#include <oomph/util/unsafe_shared_ptr.hpp>

namespace oomph
{

class communicator_impl;

namespace detail
{
// fwd declarations
struct request_state;
struct shared_request_state;

struct multi_request_state
{
    communicator_impl*            m_comm;
    std::size_t                   m_counter;
    std::vector<rank_type>        m_neighs = std::vector<rank_type>();
    std::vector<tag_type>         m_tags = std::vector<tag_type>();
    std::size_t                   m_msg_size = 0ul;
    void*                         m_msg_ptr = nullptr;
    oomph::detail::message_buffer m_msg = oomph::detail::message_buffer();
};
} // namespace detail

class send_request
{
  protected:
    using state_type = detail::request_state;
    friend class communicator;
    friend class communicator_impl;

    util::unsafe_shared_ptr<state_type> m;

    send_request(util::unsafe_shared_ptr<state_type> s) noexcept
    : m{std::move(s)}
    {
    }

  public:
    send_request() = default;
    send_request(send_request const&) = delete;
    send_request(send_request&&) = default;
    send_request& operator=(send_request const&) = delete;
    send_request& operator=(send_request&&) = default;

  public:
    bool is_ready() const noexcept;
    bool test();
    void wait();
};

class recv_request
{
  protected:
    using state_type = detail::request_state;
    friend class communicator;
    friend class communicator_impl;

    util::unsafe_shared_ptr<state_type> m;

    recv_request(util::unsafe_shared_ptr<state_type> s) noexcept
    : m{std::move(s)}
    {
    }

  public:
    recv_request() = default;
    recv_request(recv_request const&) = delete;
    recv_request(recv_request&&) = default;
    recv_request& operator=(recv_request const&) = delete;
    recv_request& operator=(recv_request&&) = default;

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

class send_multi_request
{
  protected:
    using state_type = detail::multi_request_state;
    friend class communicator;
    friend class communicator_impl;

    util::unsafe_shared_ptr<state_type> m;

    send_multi_request(util::unsafe_shared_ptr<state_type> s) noexcept
    : m{std::move(s)}
    {
    }

  public:
    send_multi_request() = default;
    send_multi_request(send_multi_request const&) = delete;
    send_multi_request(send_multi_request&&) = default;
    send_multi_request& operator=(send_multi_request const&) = delete;
    send_multi_request& operator=(send_multi_request&&) = default;

  public:
    bool is_ready() const noexcept;
    bool test();
    void wait();
};

} // namespace oomph
