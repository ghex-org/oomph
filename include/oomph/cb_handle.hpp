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

#include <oomph/detail/cb_handle.hpp>

namespace oomph
{
//class send_cb_handle
//{
//  private:
//    using data_type = detail::cb_handle_data;
//    friend class communicator;
//    friend class communicator_impl;
//
//    std::shared_ptr<data_type> m_data;
//
//    send_cb_handle(std::shared_ptr<data_type>&& data) noexcept
//    : m_data{std::move(data)}
//    {
//    }
//
//  public:
//    send_cb_handle() = default;
//    send_cb_handle(send_cb_handle const&) = delete;
//    send_cb_handle(send_cb_handle&&) = default;
//    send_cb_handle& operator=(send_cb_handle const&) = delete;
//    send_cb_handle& operator=(send_cb_handle&&) = default;
//
//  public:
//    bool is_ready() const noexcept
//    {
//        if (!m_data) return false;
//        return m_data->m_ready;
//    }
//};
//
//class recv_cb_handle
//{
//  private:
//    using data_type = detail::cb_handle_data;
//    friend class communicator;
//    friend class communicator_impl;
//
//    std::shared_ptr<data_type> m_data;
//
//    recv_cb_handle(std::shared_ptr<data_type>&& data) noexcept
//    : m_data{std::move(data)}
//    {
//    }
//
//  public:
//    recv_cb_handle() = default;
//    recv_cb_handle(recv_cb_handle const&) = delete;
//    recv_cb_handle(recv_cb_handle&&) = default;
//    recv_cb_handle& operator=(recv_cb_handle const&) = delete;
//    recv_cb_handle& operator=(recv_cb_handle&&) = default;
//
//  public:
//    bool is_ready() const noexcept
//    {
//        if (!m_data) return false;
//        return m_data->m_ready;
//    }
//    bool cancel();
//};

} // namespace oomph
