#include "./context.hpp"
#include "./communicator.hpp"
#include "./message_buffer.hpp"
#include "./request.hpp"

namespace oomph
{
template<>
region
register_memory<context_impl>(context_impl& c, void* ptr, std::size_t size)
{
    return c.make_region(ptr, size);
}

#if HWMALLOC_ENABLE_DEVICE
template<>
region
register_device_memory<context_impl>(context_impl& c, void* ptr, std::size_t size)
{
    return c.make_region(ptr, size);
}
#endif

template<>
request::request<MPI_Request>(MPI_Request r)
: m_impl(r)
{
}

request::impl::~impl() = default;

communicator::impl*
context_impl::get_communicator()
{
    auto comm = new communicator::impl{this};
    m_comms_set.insert(comm);
    return comm;
}

} // namespace oomph

#include "../src.cpp"
