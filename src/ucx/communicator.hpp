#pragma once

#include <oomph/communicator.hpp>
#include "./request.hpp"
#include "./context.hpp"

namespace oomph
{
class communicator::impl
{
  public:
    context_impl* m_context;

    auto rank() const noexcept { return m_context->rank(); }
    auto size() const noexcept { return m_context->size(); }

    void release() { m_context->deregister_communicator(this); }

    auto& get_heap() noexcept { return m_context->get_heap(); }

    auto mpi_comm() const noexcept { return m_context->get_comm(); }

    request send(
        context_impl::heap_type::pointer const& ptr, std::size_t size, rank_type dst, tag_type tag)
    {
        return {};
    }

    request recv(
        context_impl::heap_type::pointer& ptr, std::size_t size, rank_type src, tag_type tag)
    {
        return {};
    }

    void send(detail::message_buffer&& msg, std::size_t size, rank_type dst, tag_type tag,
        std::function<void(detail::message_buffer, rank_type, tag_type)>&& cb)
    {
    }

    void recv(detail::message_buffer&& msg, std::size_t size, rank_type src, tag_type tag,
        std::function<void(detail::message_buffer, rank_type, tag_type)>&& cb)
    {
    }

    void progress() {}
};

} // namespace oomph
