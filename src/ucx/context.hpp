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
    using heap_type = hwmalloc::heap<context::impl>;

  private:
    ucp_context_h m_ucp_context;
    heap_type     m_heap;

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

    auto& get_heap() noexcept { return m_heap; }
};

} // namespace oomph
