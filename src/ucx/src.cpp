#include "./context.hpp"
#include "./communicator.hpp"
#include "./message_buffer.hpp"

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
    return c.make_region(ptr, size, true);
}
#endif
} // namespace oomph

#include "../src.cpp"
