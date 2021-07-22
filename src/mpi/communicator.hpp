#pragma once

#include <oomph/communicator.hpp>
#include <oomph/context.hpp>
#include "./context.hpp"
#include "./request.hpp"
#include "../callback_utils.hpp"

namespace oomph
{
class communicator::impl
{
  public:
    context_impl* m_context;

    callback_queue<request::impl> m_callbacks;

    impl(context_impl* ctxt)
    : m_context(ctxt)
    {
    }

    auto rank() const noexcept { return m_context->rank(); }
    auto size() const noexcept { return m_context->size(); }

    void release() { m_context->deregister_communicator(this); }

    auto& get_heap() noexcept { return m_context->get_heap(); }

    auto mpi_comm() const noexcept { return m_context->get_comm(); }

    request::impl send(
        context_impl::heap_type::pointer const& ptr, std::size_t size, rank_type dst, tag_type tag)
    {
        MPI_Request r;
        OOMPH_CHECK_MPI_RESULT(MPI_Isend(ptr.get(), size, MPI_BYTE, dst, tag, mpi_comm(), &r));
        return {r};
    }

    request::impl recv(
        context_impl::heap_type::pointer& ptr, std::size_t size, rank_type src, tag_type tag)
    {
        MPI_Request r;
        OOMPH_CHECK_MPI_RESULT(MPI_Irecv(ptr.get(), size, MPI_BYTE, src, tag, mpi_comm(), &r));
        return {r};
    }

    void send(detail::message_buffer&& msg, std::size_t size, rank_type dst, tag_type tag,
        std::function<void(detail::message_buffer, rank_type, tag_type)>&& cb);

    void recv(detail::message_buffer&& msg, std::size_t size, rank_type src, tag_type tag,
        std::function<void(detail::message_buffer, rank_type, tag_type)>&& cb);

    void progress() { m_callbacks.progress(); }
};

} // namespace oomph
