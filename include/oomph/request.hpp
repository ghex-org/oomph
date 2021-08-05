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
#include <oomph/detail/cb_handle.hpp>

namespace oomph
{
class request
{
  public:
    class impl;
//
//  private:
//    using pimpl = util::pimpl<impl, 16, 8>;
//
//  public:
//    pimpl m_impl;
//
//    request();
//
//    request(request::impl&& r);
//
//    template<typename... Args>
//    request(Args... args);
//
//    request(request&&);
//
//    request& operator=(request&&);
//
//    ~request();
//
//  private:
//  public:
//    bool is_ready();
//    void wait();
//    bool cancel();
};

class send_request
{
  private:
    using data_type = detail::cb_handle_data;
    friend class communicator;
    friend class communicator_impl;

    std::shared_ptr<data_type> m_data;

    send_request(std::shared_ptr<data_type>&& data) noexcept
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
    using data_type = detail::cb_handle_data;
    friend class communicator;
    friend class communicator_impl;

    std::shared_ptr<data_type> m_data;

    recv_request(std::shared_ptr<data_type>&& data) noexcept
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
