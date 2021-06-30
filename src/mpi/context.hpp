#pragma once

#include <oomph/context.hpp>
#include "../util/heap_pimpl_impl.hpp"
#include "./region.hpp"
#include <hwmalloc/register.hpp>
#include <hwmalloc/heap.hpp>

namespace oomph
{
class context::impl
{
  public:
    using region_type = region;
    using device_region_type = region;

  private:
    MPI_Comm                      m_comm;
    MPI_Win                       m_win;
    hwmalloc::heap<context::impl> m_heap;

  public:
    impl(MPI_Comm comm)
    : m_comm{comm}
    , m_heap{this}
    {
        MPI_Info info;
        OOMPH_CHECK_MPI_RESULT(MPI_Info_create(&info));
        OOMPH_CHECK_MPI_RESULT(MPI_Info_set(info, "no_locks", "false"));
        OOMPH_CHECK_MPI_RESULT(MPI_Win_create_dynamic(info, m_comm, &m_win));
        MPI_Info_free(&info);
        //MPI_Win_create_dynamic(MPI_INFO_NULL, m_comm, &m_win);
        OOMPH_CHECK_MPI_RESULT(MPI_Win_fence(0, m_win));
    }
    impl(impl const&) = delete;
    impl(impl&&) = delete;
    ~impl() { MPI_Win_free(&m_win); }

    region make_region(void* ptr, std::size_t size) const { return {m_comm, m_win, ptr, size}; }

    auto get_window() const noexcept { return m_win; }
    auto get_comm() const noexcept { return m_comm; }
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
    return c.make_region(ptr, size);
}
#endif

} // namespace oomph
