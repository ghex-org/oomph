#pragma once

#include "./handle.hpp"

namespace oomph
{
class region
{
  public:
    using handle_type = handle;

  private:
    ucp_context_h m_ucp_context;
    void*         m_ptr;
    std::size_t   m_size;
    ucp_mem_h     m_memh;

  public:
    region(ucp_context_h ctxt, void* ptr, std::size_t size, bool gpu = false)
    : m_ucp_context{ctxt}
    , m_ptr{ptr}
    , m_size{size}
    {
    }
    region(region const&) = delete;
    region(region&& r) noexcept
    : m_ucp_context{r.m_ucp_context}
    , m_ptr{std::exchange(r.m_ptr, nullptr)}
    , m_size{r.m_size}
    , m_memh{r.m_memh}
    {
    }
    ~region()
    {
        if (m_ptr) { ucp_mem_unmap(m_ucp_context, m_memh); }
    }

    // get a handle to some portion of the region
    handle_type get_handle(std::size_t offset, std::size_t size)
    {
        return {(void*)((char*)m_ptr + offset), size};
    }
};

} // namespace oomph
