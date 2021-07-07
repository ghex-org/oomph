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

    void release() { m_context->deregister_communicator(this); }

    auto& get_heap() noexcept { return m_context->get_heap(); }

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
};

} // namespace oomph
