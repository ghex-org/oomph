#pragma once

#include <oomph/util/pimpl.hpp>
#include <cstddef>

namespace oomph
{
class context;
class communicator;

template<typename T>
class message_buffer;

namespace detail
{
class message_buffer
{
  private:
    template<typename T>
    friend class ::oomph::message_buffer;

  public:
    class heap_ptr_impl;
    using pimpl = util::pimpl<heap_ptr_impl, 64, 8>;

  private:
    void* m_ptr;
    pimpl m_heap_ptr;

  public:
    template<typename VoidPtr>
    message_buffer(VoidPtr ptr);
    message_buffer(message_buffer&&);
    ~message_buffer();
};

} // namespace detail

template<typename T>
class message_buffer
{
  private:
    friend class context;
    friend class communicator;

  private:
    detail::message_buffer m;
    std::size_t            m_size;

  private:
    message_buffer(detail::message_buffer&& m_, std::size_t size_)
    : m{std::move(m_)}
    , m_size{size_}
    {
    }

  public:
    message_buffer(message_buffer&&) = default;

  public:
    std::size_t size() const noexcept { return m_size; }

    T*       data() noexcept { return (T*)m.m_ptr; }
    T const* data() const noexcept { return (T const*)m.m_ptr; }
    T*       begin() noexcept { return data(); }
    T const* begin() const noexcept { return data(); }
    T*       end() noexcept { return data() + size(); }
    T const* end() const noexcept { return data() + size(); }
    T const* cbegin() const noexcept { return data(); }
    T const* cend() const noexcept { return data() + size(); }
};

} // namespace oomph
