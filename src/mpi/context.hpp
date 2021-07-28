#pragma once

#include "../context_base.hpp"
#include "./region.hpp"
#include "./lock_cache.hpp"
#include <hwmalloc/register.hpp>
#include <hwmalloc/heap.hpp>

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
    struct mpi_win_holder
    {
        MPI_Win m;
        ~mpi_win_holder() { MPI_Win_free(&m); }
    };

  private:
    mpi_win_holder              m_win;
    heap_type                   m_heap;
    std::unique_ptr<lock_cache> m_lock_cache;

  public:
    context_impl(MPI_Comm comm)
    : context_base(comm)
    , m_heap{this}
    {
        MPI_Info info;
        OOMPH_CHECK_MPI_RESULT(MPI_Info_create(&info));
        OOMPH_CHECK_MPI_RESULT(MPI_Info_set(info, "no_locks", "false"));
        OOMPH_CHECK_MPI_RESULT(MPI_Win_create_dynamic(info, m_mpi_comm, &(m_win.m)));
        MPI_Info_free(&info);
        OOMPH_CHECK_MPI_RESULT(MPI_Win_fence(0, m_win.m));
        m_lock_cache = std::make_unique<lock_cache>(m_win.m);
    }
    context_impl(context_impl const&) = delete;
    context_impl(context_impl&&) = delete;

    region make_region(void* ptr, std::size_t size) const
    {
        return {m_mpi_comm.get(), m_win.m, ptr, size};
    }

    auto  get_window() const noexcept { return m_win.m; }
    auto& get_heap() noexcept { return m_heap; }

    communicator::impl* get_communicator();

    void lock(communicator::rank_type r) { m_lock_cache->lock(r); }
};

} // namespace oomph
