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
    using heap_type = hwmalloc::heap<context::impl>;

  private:
    struct mpi_win_holder
    {
        MPI_Win m;
        ~mpi_win_holder() { MPI_Win_free(&m); }
    };

  private:
    MPI_Comm       m_comm;
    mpi_win_holder m_win;
    heap_type      m_heap;

  public:
    impl(MPI_Comm comm)
    : m_comm{comm}
    , m_heap{this}
    {
        MPI_Info info;
        OOMPH_CHECK_MPI_RESULT(MPI_Info_create(&info));
        OOMPH_CHECK_MPI_RESULT(MPI_Info_set(info, "no_locks", "false"));
        OOMPH_CHECK_MPI_RESULT(MPI_Win_create_dynamic(info, m_comm, &(m_win.m)));
        MPI_Info_free(&info);
        //MPI_Win_create_dynamic(MPI_INFO_NULL, m_comm, &m_win);
        OOMPH_CHECK_MPI_RESULT(MPI_Win_fence(0, m_win.m));
    }
    impl(impl const&) = delete;
    impl(impl&&) = delete;

    region make_region(void* ptr, std::size_t size) const { return {m_comm, m_win.m, ptr, size}; }

    auto  get_window() const noexcept { return m_win.m; }
    auto  get_comm() const noexcept { return m_comm; }
    auto& get_heap() noexcept { return m_heap; }
};

} // namespace oomph
