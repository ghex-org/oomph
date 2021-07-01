#pragma once

#include <oomph/context.hpp>
#include "../util/heap_pimpl_impl.hpp"
#include "../communicator_holder.hpp"
#include "./region.hpp"
#include <hwmalloc/register.hpp>
#include <hwmalloc/register_device.hpp>
#include <hwmalloc/heap.hpp>

namespace oomph
{
class context_impl
{
  public:
    using region_type = region;
    using device_region_type = region;
    using heap_type = hwmalloc::heap<context_impl>;

  private:
    ucp_context_h m_ucp_context;
    heap_type     m_heap;
    tl_comm_holder m_comms;

  public:
    context_impl(MPI_Comm)
    : m_heap{this}
    {
    }
    context_impl(context_impl&&) = default;
    context_impl& operator=(context_impl&&) = default;

    region make_region(void* ptr, std::size_t size, bool gpu = false)
    {
        return {m_ucp_context, ptr, size, gpu};
    }

    auto& get_heap() noexcept { return m_heap; }

    void add_communicator(communicator::impl** c) { m_comms.insert(c); }
    void remove_communicator(communicator::impl** c) { m_comms.remove(c); }
};

} // namespace oomph
