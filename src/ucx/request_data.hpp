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

#include "./request_state.hpp"

namespace oomph
{
class communicator_impl;

struct request_data
{
    detail::request_state*        m_req;
    detail::shared_request_state* m_shared_req;
    //bool                          m_empty;

    void destroy()
    {
        //m_comm = nullptr;
        //m_cb.~cb_t();
        //m_empty = true;
        m_req = nullptr;
        m_shared_req = nullptr;
    }

    bool empty() const noexcept { return !((bool)m_req || (bool)m_shared_req); }

    static request_data* construct(void* ptr, detail::request_state* req)
    {
        return ::new (get_impl(ptr)) request_data{req, nullptr};
    }

    static request_data* construct(void* ptr, detail::shared_request_state* req)
    {
        return ::new (get_impl(ptr)) request_data{nullptr, req};
    }

    // return pointer to an instance from ucx provided storage pointer
    static request_data* get(void* ptr) { return std::launder(get_impl(ptr)); }

    // initialize request on pristine request data allocated by ucx
    static void init(void* ptr) { get(ptr)->destroy(); }

  private:
    static request_data* get_impl(void* ptr)
    {
        // alignment mask
        static constexpr std::uintptr_t mask = ~(alignof(request_data) - 1u);
        return reinterpret_cast<request_data*>(
            (reinterpret_cast<std::uintptr_t>((unsigned char*)ptr) + alignof(request_data) - 1) &
            mask);
    }
};

using request_data_size =
    std::integral_constant<std::size_t, sizeof(request_data) + alignof(request_data)>;

} // namespace oomph
