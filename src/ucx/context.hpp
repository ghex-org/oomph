#pragma once

#include <oomph/context.hpp>
#include "../unique_ptr_set.hpp"
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
    ucp_context_h                      m_ucp_context;
    heap_type                          m_heap;
    unique_ptr_set<communicator::impl> m_comms_set;

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

    communicator::impl* get_communicator();

    void deregister_communicator(communicator::impl* c) { m_comms_set.remove(c); }
};

} // namespace oomph
