#pragma once

#include <oomph/util/moved_bit.hpp>
#include <oomph/util/mpi_clone_comm.hpp>
#include <oomph/util/heap_pimpl.hpp>
#include <oomph/message_buffer.hpp>
#include <hwmalloc/config.hpp>

namespace oomph
{
class context
{
  public:
    class impl;
    using pimpl = util::heap_pimpl<impl>;

  private:
    class mpi_comm_holder
    {
      private:
        MPI_Comm        m;
        util::moved_bit m_moved;

      public:
        mpi_comm_holder(MPI_Comm comm)
        : m{util::mpi_clone_comm(comm)}
        {
        }
        mpi_comm_holder(mpi_comm_holder const&) = delete;
        mpi_comm_holder(mpi_comm_holder&&) noexcept = default;
        mpi_comm_holder& operator=(mpi_comm_holder const&) = delete;
        mpi_comm_holder& operator=(mpi_comm_holder&&) noexcept = default;
        ~mpi_comm_holder() noexcept
        {
            if (!m_moved) MPI_Comm_free(&m);
        }
        MPI_Comm get() const noexcept { return m; }
    };

  private:
    mpi_comm_holder m_mpi_comm;
    pimpl           m;

  public:
    context(MPI_Comm comm);
    context(context const&) = delete;
    context(context&&);
    context& operator=(context const&) = delete;
    context& operator=(context&&);
    ~context();

  public:
    MPI_Comm mpi_comm() const noexcept { return m_mpi_comm.get(); }

    template<typename T>
    message_buffer<T> make_buffer(std::size_t size)
    {
        return {make_buffer_core(size * sizeof(T)), size};
    }

  private:
    detail::message_buffer make_buffer_core(std::size_t size);
};

template<typename Context>
typename Context::region_type register_memory(Context&, void*, std::size_t);
#if HWMALLOC_ENABLE_DEVICE
template<typename Context>
typename Context::device_region_type register_device_memory(Context&, void*, std::size_t);
#endif

} // namespace oomph
