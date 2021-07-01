#include <hwmalloc/numa.hpp>
namespace oomph
{
namespace detail
{
using heap_ptr = typename context::impl::heap_type::pointer;

class message_buffer::heap_ptr_impl
{
  public:
    heap_ptr m;
    void     release() { m.release(); }
};

template<>
message_buffer::message_buffer<typename context::impl::heap_type::pointer>(
    typename context::impl::heap_type::pointer ptr)
: m_ptr{ptr.get()}
, m_heap_ptr(ptr)
{
}

message_buffer::message_buffer(message_buffer&& other)
: m_ptr{std::exchange(other.m_ptr, nullptr)}
, m_heap_ptr{std::move(other.m_heap_ptr)}
{
}

message_buffer::~message_buffer()
{
    if (m_ptr) m_heap_ptr->release();
}

} // namespace detail

detail::message_buffer
context::make_buffer_core(std::size_t size)
{
    return {m->get_heap().allocate(size, hwmalloc::numa().local_node())};
}

} // namespace oomph
