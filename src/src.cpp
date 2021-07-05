#include <hwmalloc/numa.hpp>

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
    auto context_impl_ptr = &(*m);
    return {context_impl_ptr};
}

///////////////////////////////
// thread_local communicator //
///////////////////////////////

communicator::impl*
get_tl_comm(context_impl* c)
{
    static thread_local tl_comm tl_c(c);
    return tl_c.m_communicator;
}

tl_comm::tl_comm(context_impl* c)
: m_context{c}
, m_communicator{new communicator::impl{m_context}}
{
    m_context->add_communicator(&m_communicator);
}

tl_comm::~tl_comm()
{
    if (m_communicator) m_context->remove_communicator(&m_communicator);
}

///////////////////////////////
// communicator              //
///////////////////////////////

communicator::communicator(context_impl* c_impl_)
: m_context_impl{c_impl_}
{
    get_impl();
}

communicator::impl*
communicator::get_impl()
{
    return get_tl_comm(m_context_impl);
}

///////////////////////////////
// communicator_holder       //
///////////////////////////////

void
tl_comm_holder::destroy(communicator::impl** c)
{
    delete (*c);
    *c = nullptr;
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
    return {m_context_impl->get_heap().allocate(size, hwmalloc::numa().local_node())};
}

} // namespace oomph
