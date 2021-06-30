#pragma once

#include <oomph/context.hpp>
#include "../util/heap_pimpl_impl.hpp"
#include "./region.hpp"
#include <hwmalloc/register.hpp>
#include <hwmalloc/register_device.hpp>
#include <hwmalloc/heap.hpp>

namespace oomph
{
class context::impl
{
  public:
    using region_type = region;
    using device_region_type = region;

  private:
    ucp_context_h                 m_ucp_context;
    hwmalloc::heap<context::impl> m_heap;

  public:
    impl(MPI_Comm)
    : m_heap{this}
    {
    }
    impl(impl&&) = default;
    impl& operator=(impl&&) = default;

    region make_region(void* ptr, std::size_t size, bool gpu = false)
    {
        return {m_ucp_context, ptr, size, gpu};
    }
};

template<>
region
register_memory<context::impl>(context::impl& c, void* ptr, std::size_t size)
{
    return c.make_region(ptr, size);
}
#if HWMALLOC_ENABLE_DEVICE
template<>
region
register_device_memory<context::impl>(context::impl& c, void* ptr, std::size_t size)
{
    return c.make_region(ptr, size, true);
}
#endif

} // namespace oomph
