/*
 * GridTools
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <oomph/message_buffer.hpp>
#include <oomph/request.hpp>
#include <oomph/util/mpi_error.hpp>
#include <hwmalloc/device.hpp>
#include <functional>
#include <vector>
#include <atomic>
#include <cassert>
#include <boost/callable_traits.hpp>

#define OOMPH_CHECK_CALLBACK_F(CALLBACK, RANK_TYPE)                                                \
    {                                                                                              \
        using args_t = boost::callable_traits::args_t<std::remove_reference_t<CALLBACK>>;          \
        using arg0_t = std::tuple_element_t<0, args_t>;                                            \
        using arg1_t = std::tuple_element_t<1, args_t>;                                            \
        using arg2_t = std::tuple_element_t<2, args_t>;                                            \
        static_assert(std::tuple_size<args_t>::value == 3, "callback must have 3 arguments");      \
        static_assert(std::is_same<arg1_t, RANK_TYPE>::value,                                      \
            "rank_type is not convertible to second callback argument type");                      \
        static_assert(std::is_same<arg2_t, tag_type>::value,                                       \
            "tag_type is not convertible to third callback argument type");                        \
        using TT = typename std::remove_reference_t<arg0_t>::value_type;                           \
        static_assert(std::is_same<arg0_t, message_buffer<TT>>::value,                             \
            "first callback argument type is not a message_buffer");                               \
        static_assert(std::is_same<arg0_t, message_buffer<TT>>::value,                             \
            "first callback argument type is not a message_buffer");                               \
    }

#define OOMPH_CHECK_CALLBACK(CALLBACK) OOMPH_CHECK_CALLBACK_F(CALLBACK, rank_type)

#define OOMPH_CHECK_CALLBACK_MULTI(CALLBACK)                                                       \
    OOMPH_CHECK_CALLBACK_F(CALLBACK, std::vector<rank_type>)

namespace oomph
{
class context;
class send_channel_base;
class recv_channel_base;

class communicator
{
  public:
    class impl;
    using rank_type = int;
    using tag_type = int;

  public:
    static constexpr rank_type any_source = -1;
    static constexpr tag_type  any_tag = -1;

  private:
    friend class context;
    friend class send_channel_base;
    friend class recv_channel_base;

  private:
    impl* m_impl;

  private:
    communicator(impl* impl_);

  public:
    communicator(communicator const&) = delete;
    communicator(communicator&&);
    communicator& operator=(communicator const&) = delete;
    communicator& operator=(communicator&&);
    ~communicator();

  public:
    rank_type rank() const noexcept;
    rank_type size() const noexcept;
    MPI_Comm  mpi_comm() const noexcept;
    bool      is_local(rank_type rank) const noexcept;

    template<typename T>
    message_buffer<T> make_buffer(std::size_t size)
    {
        return {make_buffer_core(size * sizeof(T)), size};
    }

#if HWMALLOC_ENABLE_DEVICE
    template<typename T>
    message_buffer<T> make_device_buffer(std::size_t size, int id = hwmalloc::get_device_id())
    {
        return {make_buffer_core(size * sizeof(T)), size, id};
    }
#endif

    template<typename T>
    [[nodiscard]] request send(message_buffer<T> const& msg, rank_type dst, tag_type tag)
    {
        assert(msg);
        return send(msg.m, msg.size() * sizeof(T), dst, tag);
    }

    template<typename T>
    [[nodiscard]] request recv(message_buffer<T>& msg, rank_type src, tag_type tag)
    {
        assert(msg);
        return recv(msg.m, msg.size() * sizeof(T), src, tag);
    }

    template<typename T, typename CallBack>
    void send(message_buffer<T>&& msg, rank_type dst, tag_type tag, CallBack&& callback)
    {
        OOMPH_CHECK_CALLBACK(CallBack)
        assert(msg);
        const auto s = msg.size();
        send(std::move(msg.m), s * sizeof(T), dst, tag,
            [s, cb = std::forward<CallBack>(callback)](detail::message_buffer m, rank_type dst,
                tag_type tag) mutable { cb(message_buffer<T>(std::move(m), s), dst, tag); });
    }

    template<typename T, typename CallBack>
    void recv(message_buffer<T>&& msg, rank_type src, tag_type tag, CallBack&& callback)
    {
        OOMPH_CHECK_CALLBACK(CallBack)
        assert(msg);
        const auto s = msg.size();
        recv(std::move(msg.m), s * sizeof(T), src, tag,
            [s, cb = std::forward<CallBack>(callback)](detail::message_buffer m, rank_type src,
                tag_type tag) mutable { cb(message_buffer<T>(std::move(m), s), src, tag); });
    }

    template<typename T>
    [[nodiscard]] std::vector<request> send_multi(
        message_buffer<T> const& msg, std::vector<rank_type> const& neighs, tag_type tag)
    {
        assert(msg);
        std::vector<request> reqs;
        reqs.reserve(neighs.size());
        for (auto id : neighs) reqs.push_back(send(msg, id, tag));
        return reqs;
    }

    template<typename T, typename CallBack>
    void send_multi(message_buffer<T>&& msg, std::vector<rank_type> const& neighs, tag_type tag,
        CallBack&& callback)
    {
        OOMPH_CHECK_CALLBACK_MULTI(CallBack)
        assert(msg);
        const auto s = msg.size();
        auto       counter = new std::atomic<int>(neighs.size());
        auto       neighs_ptr = new std::vector<rank_type>(neighs);
        const auto n_neighs = neighs.size();
        auto       cb = std::forward<CallBack>(callback);

        auto multi_cb = [s, counter, neighs_ptr, cb](
                            detail::message_buffer m, rank_type, tag_type tag) mutable {
            if ((--(*counter)) == 0)
            {
                cb(message_buffer<T>(std::move(m), s), *neighs_ptr, tag);
                delete counter;
                delete neighs_ptr;
            }
            else
            {
                m.clear();
            }
        };

        std::size_t i = 0;
        for (auto id : neighs)
        {
            if ((i + 1) < n_neighs) send(clone_buffer(msg.m), s * sizeof(T), id, tag, multi_cb);
            else
                send(std::move(msg.m), s * sizeof(T), id, tag, multi_cb);
            ++i;
        }
    }

    template<typename CallBack>
    bool cancel_recv_cb(rank_type src, tag_type tag, CallBack&& callback)
    {
        OOMPH_CHECK_CALLBACK(CallBack)
        using args_t = boost::callable_traits::args_t<std::remove_reference_t<CallBack>>;
        using msg_t = std::tuple_element_t<0, args_t>;
        using value_t = typename msg_t::value_type;
        return cancel_recv_cb_(src, tag,
            [cb = std::forward<CallBack>(callback)](
                detail::message_buffer m, std::size_t size, rank_type dst, tag_type tag) mutable {
                cb(msg_t{std::move(m), size / sizeof(value_t)}, dst, tag);
            });
    }

    void progress();

  private:
    detail::message_buffer make_buffer_core(std::size_t size);
#if HWMALLOC_ENABLE_DEVICE
    detail::message_buffer make_buffer_core(std::size_t size, int device_id);
#endif

    request send(detail::message_buffer const& msg, std::size_t size, rank_type dst, tag_type tag);
    request recv(detail::message_buffer& msg, std::size_t size, rank_type src, tag_type tag);

    void send(detail::message_buffer&& msg, std::size_t size, rank_type dst, tag_type tag,
        std::function<void(detail::message_buffer, rank_type, tag_type)>&& cb);

    void recv(detail::message_buffer&& msg, std::size_t size, rank_type src, tag_type tag,
        std::function<void(detail::message_buffer, rank_type, tag_type)>&& cb);

    detail::message_buffer clone_buffer(detail::message_buffer& msg);

    bool cancel_recv_cb_(rank_type src, tag_type tag,
        std::function<void(detail::message_buffer, std::size_t size, rank_type, tag_type)>&& cb);
};

} // namespace oomph
