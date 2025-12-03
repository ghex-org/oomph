/*
 * ghex-org
 *
 * Copyright (c) 2014-2023, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <hwmalloc/heap.hpp>
#include <hwmalloc/heap_config.hpp>

#include <oomph/config.hpp>

// paths relative to backend
#include <nccl_communicator.hpp>
#include <../context_base.hpp>
#include <region.hpp>

namespace oomph
{
class context_impl : public context_base
{
  public:
    using region_type = region;
    using device_region_type = region;
    using heap_type = hwmalloc::heap<context_impl>;

  private:
    heap_type m_heap;
    detail::nccl_comm m_comm;

  public:
    context_impl(MPI_Comm comm, bool thread_safe, hwmalloc::heap_config const& heap_config)
    : context_base(comm, thread_safe)
    , m_heap{this, heap_config}
    , m_comm{oomph::detail::nccl_comm{comm}}
    {
    }

    context_impl(context_impl const&) = delete;
    context_impl(context_impl&&) = delete;

    ncclComm_t get_comm() const noexcept { return m_comm.get(); }

    region make_region(void* ptr) const { return {ptr}; }

    auto& get_heap() noexcept { return m_heap; }

    communicator_impl* get_communicator();

    void progress() {
        // NCCL will make progress on its own. Or deadlock.
    }

    bool cancel_recv(detail::shared_request_state*) {
      // TODO: Ignore? Can't undo kernel launches.
      return false;
    }

    unsigned int num_tag_bits() const noexcept {
      // TODO: Important? Can't use tags with NCCL.
      return 32;
    }

    const char* get_transport_option(const std::string& opt);
};

template<>
inline region
register_memory<context_impl>(context_impl& c, void* ptr, std::size_t)
{
    return c.make_region(ptr);
}

#if OOMPH_ENABLE_DEVICE
template<>
inline region
register_device_memory<context_impl>(context_impl& c, int, void* ptr, std::size_t)
{
    return c.make_region(ptr);
}
#endif

} // namespace oomph
