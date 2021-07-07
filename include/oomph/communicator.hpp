#pragma once

#include <oomph/message_buffer.hpp>
#include <oomph/request.hpp>
#include <cassert>

namespace oomph
{
class context;
//class context_impl;

class communicator
{
  public:
    class impl;
    using rank_type = int;
    using tag_type = int;

  private:
    friend class context;

  private:
    impl* m_impl;
    //context_impl* m_context_impl;

  private:
    communicator(impl* impl_ /*, context_impl* c_impl_*/);

  public:
    communicator(communicator const&) = delete;
    communicator(communicator&&);
    communicator& operator=(communicator const&) = delete;
    communicator& operator=(communicator&&);
    ~communicator();

  public:
    template<typename T>
    message_buffer<T> make_buffer(std::size_t size)
    {
        return {make_buffer_core(size * sizeof(T)), size};
    }

    template<typename T>
    request send(message_buffer<T> const& msg, rank_type dst, tag_type tag)
    {
        assert(msg);
        return send(msg.m, msg.size() * sizeof(T), dst, tag);
    }

    template<typename T>
    request recv(message_buffer<T>& msg, rank_type src, tag_type tag)
    {
        assert(msg);
        return recv(msg.m, msg.size() * sizeof(T), src, tag);
    }

  private:
    detail::message_buffer make_buffer_core(std::size_t size);

    request send(detail::message_buffer const& msg, std::size_t size, rank_type dst, tag_type tag);
    request recv(detail::message_buffer& msg, std::size_t size, rank_type src, tag_type tag);

    //impl* get_impl();
};

} // namespace oomph
