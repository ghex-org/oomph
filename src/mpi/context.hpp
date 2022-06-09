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
#include "./request_queue.hpp"

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
    heap_type    m_heap;
    rma_context  m_rma_context;
    unsigned int m_max_tag;
    unsigned int m_reserved_tag;

  public:
    shared_request_queue m_req_queue;

  public:
    context_impl(MPI_Comm comm, bool thread_safe, bool message_pool_never_free,
        std::size_t message_pool_reserve)
    : context_base(comm, thread_safe)
    , m_heap{this, message_pool_never_free, message_pool_reserve}
    , m_rma_context{m_mpi_comm}
    {
        // get largest allowed tag value
        int  flag;
        int* tag_ub;
        MPI_Comm_get_attr(this->get_comm(), MPI_TAG_UB, &tag_ub, &flag);
        m_max_tag = flag ? *tag_ub : 32767;

        // compute bit mask
        unsigned long tmp = m_max_tag;
        unsigned long mask = 1u;
        while (tmp > 0)
        {
            tmp >>= 1;
            mask <<= 1;
        }
        mask -= 1;

        // If bit mask is larger than max tag value, then we have some strange upper bound which is
        // not at a power of 2 boundary and we reduce the maximum to the next lower power of 2.
        if (mask > m_max_tag) mask >>= 1;

        // reduce max tag by 1 bit to make room for reserved tag
        m_max_tag = (mask >> 1);
        m_reserved_tag = m_max_tag + 1;
    }

    context_impl(context_impl const&) = delete;
    context_impl(context_impl&&) = delete;

    region make_region(void* ptr) const { return {ptr}; }

    auto& get_heap() noexcept { return m_heap; }

    auto  get_window() const noexcept { return m_rma_context.get_window(); }
    auto& get_rma_heap() noexcept { return m_rma_context.get_heap(); }
    void  lock(communicator::rank_type r) { m_rma_context.lock(r); }

    communicator_impl* get_communicator();

    void progress() { m_req_queue.progress(); }

    bool cancel_recv(detail::shared_request_state* r) { return m_req_queue.cancel(r); }

    unsigned int max_tag() const noexcept { return m_max_tag; }
    unsigned int reserved_tag() const noexcept { return m_reserved_tag; }
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
