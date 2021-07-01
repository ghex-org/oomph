#pragma once

#include <oomph/util/mpi_comm_holder.hpp>
#include <oomph/util/heap_pimpl.hpp>
#include <oomph/message_buffer.hpp>
#include <oomph/communicator.hpp>
#include <hwmalloc/config.hpp>

namespace oomph
{
class context_impl;
class context
{
  public:
    //class impl;
    using pimpl = util::heap_pimpl<context_impl>;

  private:
    util::mpi_comm_holder m_mpi_comm;
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

    communicator get_communicator();

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
