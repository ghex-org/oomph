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

#include <memory>

namespace oomph
{
class communicator;
class communicator_impl;

namespace detail
{
struct cb_handle_data
{
    communicator_impl* m_comm;
    std::size_t        m_index;
    bool               m_ready;
    void*              m_data = nullptr;

    cb_handle_data(communicator_impl* comm) noexcept
    : m_comm{comm}
    , m_index{0}
    , m_ready{false}
    {
    }

    cb_handle_data(communicator_impl* comm, std::size_t index, bool r) noexcept
    : m_comm{comm}
    , m_index{index}
    , m_ready{r}
    {
    }
};

} // namespace detail
} // namespace oomph
