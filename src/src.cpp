#include <hwmalloc/numa.hpp>
#include <oomph/util/heap_pimpl.hpp>
#include <oomph/util/stack_pimpl.hpp>
#include "./util/heap_pimpl_impl.hpp"
#include "./util/stack_pimpl_impl.hpp"

namespace oomph
{
///////////////////////////////
// context                   //
///////////////////////////////

context::context(MPI_Comm comm)
: m_mpi_comm{comm}
, m(m_mpi_comm.get())
{
}

context::context(context&&) = default;

context& context::operator=(context&&) = default;

context::~context() = default;

communicator
context::get_communicator()
{
    return {m->get_communicator()};
}

///////////////////////////////
// communicator              //
///////////////////////////////

communicator::communicator(impl* impl_)
: m_impl{impl_}
{
}

communicator::communicator(communicator&& other)
: m_impl{std::exchange(other.m_impl, nullptr)}
{
}

communicator&
communicator::operator=(communicator&& other)
{
    m_impl = std::exchange(other.m_impl, nullptr);
    return *this;
}

communicator::~communicator()
{
    if (m_impl) m_impl->release();
}

///////////////////////////////
// message_buffer            //
///////////////////////////////

namespace detail
{
using heap_ptr = typename context_impl::heap_type::pointer;

class message_buffer::heap_ptr_impl
{
  public:
    heap_ptr m;
    void     release() { m.release(); }
};

message_buffer::message_buffer() = default;

template<>
message_buffer::message_buffer<typename context_impl::heap_type::pointer>(
    typename context_impl::heap_type::pointer ptr)
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

message_buffer&
message_buffer::operator=(message_buffer&& other)
{
    if (m_ptr) m_heap_ptr->release();
    m_ptr = std::exchange(other.m_ptr, nullptr);
    m_heap_ptr = std::move(other.m_heap_ptr);
    return *this;
}

} // namespace detail

request
communicator::send(detail::message_buffer const& msg, std::size_t size, rank_type dst, tag_type tag)
{
    return m_impl->send(msg.m_heap_ptr->m, size, dst, tag);
}

request
communicator::recv(detail::message_buffer& msg, std::size_t size, rank_type src, tag_type tag)
{
    return m_impl->recv(msg.m_heap_ptr->m, size, src, tag);
}

///////////////////////////////
// make_buffer               //
///////////////////////////////

detail::message_buffer
context::make_buffer_core(std::size_t size)
{
    return {m->get_heap().allocate(size, hwmalloc::numa().local_node())};
}

detail::message_buffer
communicator::make_buffer_core(std::size_t size)
{
    return {m_impl->get_heap().allocate(size, hwmalloc::numa().local_node())};
}

///////////////////////////////
// request                   //
///////////////////////////////

bool
request::is_ready()
{
    return m_impl->is_ready();
}

void
request::wait()
{
    return m_impl->wait();
}

request::~request() = default;

request::request(request&&) = default;

/////////////////////////////////
//// send_channel_base         //
/////////////////////////////////
//
//send_channel_base::~send_channel_base() = default;
//
//void
//send_channel_base::connect()
//{
//    m_impl->connect();
//}
//
/////////////////////////////////
//// recv_channel_base         //
/////////////////////////////////
//
//recv_channel_base::~recv_channel_base() = default;
//
//void
//recv_channel_base::connect()
//{
//    m_impl->connect();
//}
//
//std::size_t
//recv_channel_base::capacity()
//{
//    return m_impl->capacity();
//}
//
//void*
//recv_channel_base::get(std::size_t& index)
//{
//    return m_impl->get(index);
//}
//
//recv_channel_impl*
//recv_channel_base::get_impl() noexcept
//{
//    return &(*m_impl);
//}

} // namespace oomph
