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

#include "../context_base.hpp"
#include "./rma_context.hpp"
//#include "./shared_callback_queue.hpp"
#include "./request_queue.hpp"
#include <atomic>

namespace oomph
{
class context_impl : public context_base
{
  public:
    using region_type = region;
    using device_region_type = region;
    using heap_type = hwmalloc::heap<context_impl>;
    using rank_type = communicator::rank_type;
    using tag_type = communicator::tag_type;

  private:
    heap_type   m_heap;
    rma_context m_rma_context;

  public:
    //shared_callback_queue m_cb_queue;
    shared_request_queue m_req_queue;
  public:
    std::atomic<std::size_t> m_scheduled;

  public:
    context_impl(MPI_Comm comm, bool thread_safe, bool message_pool_never_free,
        std::size_t message_pool_reserve)
    : context_base(comm, thread_safe)
    , m_heap{this, message_pool_never_free, message_pool_reserve}
    , m_rma_context{m_mpi_comm}
    , m_scheduled(0ul)
    {
    }

    context_impl(context_impl const&) = delete;
    context_impl(context_impl&&) = delete;

    region make_region(void* ptr) const { return {ptr}; }

    auto& get_heap() noexcept { return m_heap; }

    auto  get_window() const noexcept { return m_rma_context.get_window(); }
    auto& get_rma_heap() noexcept { return m_rma_context.get_heap(); }
    void  lock(communicator::rank_type r) { m_rma_context.lock(r); }

    communicator_impl* get_communicator();

    void progress()
    {
        m_req_queue.progress();
    }
};

template<>
region
register_memory<context_impl>(context_impl& c, void* ptr, std::size_t)
{
    return c.make_region(ptr);
}

#if HWMALLOC_ENABLE_DEVICE
template<>
region
register_device_memory<context_impl>(context_impl& c, void* ptr, std::size_t)
{
    return c.make_region(ptr);
}
#endif

} // namespace oomph
