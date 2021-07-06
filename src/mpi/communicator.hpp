#pragma once

#include <oomph/communicator.hpp>
#include <oomph/context.hpp>
#include "./context.hpp"
#include "./request.hpp"

namespace oomph
{
class communicator::impl
{
  public:
    context_impl* m_context;

    void release() { m_context->deregister_communicator(this); }

    auto& get_heap() noexcept { return m_context->get_heap(); }

    request send(
        context_impl::heap_type::pointer const& ptr, std::size_t size, rank_type dst, tag_type tag)
    {
        MPI_Request r;
        auto        mpi_comm = m_context->get_comm();
        OOMPH_CHECK_MPI_RESULT(
            MPI_Isend(ptr.get(), size, MPI_BYTE, dst, tag, m_context->get_comm(), &r));
        return {r};
    }

    request recv(
        context_impl::heap_type::pointer& ptr, std::size_t size, rank_type src, tag_type tag)
    {
        MPI_Request r;
        OOMPH_CHECK_MPI_RESULT(
            MPI_Irecv(ptr.get(), size, MPI_BYTE, src, tag, m_context->get_comm(), &r));
        return {r};
    }
};

} // namespace oomph
