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

#include <oomph/util/unique_function.hpp>

namespace oomph
{
class communicator_impl;

struct request_data
{
    using comm_ptr_t = communicator_impl*;
    using cb_ptr_t = util::detail::unique_function<void>*;

    void*      m_ucx_ptr;
    comm_ptr_t m_comm;
    cb_ptr_t   m_cb;

    void clear()
    {
        m_comm = nullptr;
        m_cb = nullptr;
    }

    static request_data* construct(void* ptr, comm_ptr_t comm, cb_ptr_t cb)
    {
        // alignment mask
        static constexpr std::uintptr_t mask = ~(alignof(request_data) - 1u);
        // align pointer
        auto a_ptr = reinterpret_cast<request_data*>(
            (reinterpret_cast<std::uintptr_t>((unsigned char*)ptr) + alignof(request_data) - 1) &
            mask);
        // construct in ucx provided memory
        new (a_ptr) request_data{ptr, comm, cb};
        return a_ptr;
    }

    // return pointer to an instance from ucx provided storage pointer
    static request_data& get(void* ptr)
    {
        // alignment mask
        static constexpr std::uintptr_t mask = ~(alignof(request_data) - 1u);
        return *reinterpret_cast<request_data*>(
            (reinterpret_cast<std::uintptr_t>((unsigned char*)ptr) + alignof(request_data) - 1) &
            mask);
    }

    // initialize request on prestine request data allocated by ucx
    static void init(void* ptr) { request_data::construct(ptr, nullptr, nullptr); }
};

using request_data_size =
    std::integral_constant<std::size_t, sizeof(request_data) + alignof(request_data)>;

} // namespace oomph
