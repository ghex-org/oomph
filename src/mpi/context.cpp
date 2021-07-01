#include "./context.hpp"
namespace oomph
{
template<>
region
register_memory<context::impl>(context::impl& c, void* ptr, std::size_t size)
{
    return c.make_region(ptr, size);
}
#if HWMALLOC_ENABLE_DEVICE
template<>
region
register_device_memory<context::impl>(context::impl& c, void* ptr, std::size_t size)
{
    return c.make_region(ptr, size);
}
#endif
} // namespace oomph
#include "../context.cpp"
