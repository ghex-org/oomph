/*
 * GridTools
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "./context.hpp"
#include "./communicator.hpp"

namespace oomph
{
template<>
region
register_memory<context_impl>(context_impl& c, void* ptr, std::size_t size)
{
    return c.make_region(ptr, size);
}
#if HWMALLOC_ENABLE_DEVICE
template<>
region
register_device_memory<context_impl>(context_impl& c, void* ptr, std::size_t size)
{
    return c.make_region(ptr, size, true);
}
#endif

communicator_impl*
context_impl::get_communicator()
{
    auto comm = new communicator_impl{this, this->m_thread_safe, m_worker.get(),
        std::make_unique<worker_type>(get(), m_db,
            (this->m_thread_safe ? UCS_THREAD_MODE_SERIALIZED : UCS_THREAD_MODE_SINGLE)),
        m_mutex};
    m_comms_set.insert(comm);
    return comm;
}

} // namespace oomph

#include "../src.cpp"
