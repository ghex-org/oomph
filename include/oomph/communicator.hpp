#pragma once

#include <oomph/message_buffer.hpp>

namespace oomph
{
class context;
class context_impl;

class communicator
{
  public:
    class impl;

  private:
    friend class context;

  private:
    context_impl* m_context_impl;

  private:
    communicator(context_impl* c_impl_);

  public:
    template<typename T>
    message_buffer<T> make_buffer(std::size_t size)
    {
        return {make_buffer_core(size * sizeof(T)), size};
    }

  private:
    detail::message_buffer make_buffer_core(std::size_t size);

    impl* get_impl();
};

} // namespace oomph
