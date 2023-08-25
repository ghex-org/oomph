/*
 * ghex-org
 *
 * Copyright (c) 2014-2023, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <functional>
#include <vector>
#include <atomic>
#include <cassert>
#include <boost/callable_traits.hpp>
#include <hwmalloc/device.hpp>
#include <oomph/config.hpp>
#include <oomph/message_buffer.hpp>
#include <oomph/detail/communicator_helper.hpp>
#include <oomph/util/mpi_error.hpp>
#include <oomph/util/unique_function.hpp>

namespace oomph
{

class context;

class communicator
{
  private:
    friend class context;

  public:
    using impl_type = communicator_impl;

  public:
    static constexpr rank_type any_source = -1;
    static constexpr tag_type  any_tag = -1;

  private:
    template<typename T, typename CallBack>
    struct cb_rref
    {
        std::decay_t<CallBack> cb;
        message_buffer<T>      m;

        void operator()(rank_type r, tag_type t) { cb(std::move(m), r, t); }
    };

    template<typename T, typename CallBack>
    struct cb_lref
    {
        std::decay_t<CallBack> cb;
        message_buffer<T>*     m;

        void operator()(rank_type r, tag_type t) { cb(*m, r, t); }
    };

    template<typename T, typename CallBack>
    struct cb_lref_const
    {
        std::decay_t<CallBack>   cb;
        message_buffer<T> const* m;

        void operator()(rank_type r, tag_type t) { cb(*m, r, t); }
    };

  private:
    util::unsafe_shared_ptr<detail::communicator_state> m_state;

  private:
    communicator(impl_type* impl_, std::atomic<std::size_t>* shared_scheduled_recvs)
    : m_state{util::make_shared<detail::communicator_state>(impl_, shared_scheduled_recvs)}
    {
    }

    communicator(util::unsafe_shared_ptr<detail::communicator_state> s) noexcept
    : m_state{s}
    {
    }

  public:
    communicator(communicator const&) = delete;
    communicator(communicator&& other) = default;
    communicator& operator=(communicator const&) = delete;
    communicator& operator=(communicator&& other) = default;

  public:
    rank_type   rank() const noexcept;
    rank_type   size() const noexcept;
    MPI_Comm    mpi_comm() const noexcept;
    bool        is_local(rank_type rank) const noexcept;
    std::size_t scheduled_sends() const noexcept { return m_state->scheduled_sends; }
    std::size_t scheduled_recvs() const noexcept { return m_state->scheduled_recvs; }
    std::size_t scheduled_shared_recvs() const noexcept
    {
        return m_state->m_shared_scheduled_recvs->load();
    }

    bool is_ready() const noexcept
    {
        return (scheduled_sends() == 0) && (scheduled_recvs() == 0) &&
               (scheduled_shared_recvs() == 0);
    }

    void wait_all()
    {
        while (!is_ready()) { progress(); }
    }

    template<typename T>
    message_buffer<T> make_buffer(std::size_t size)
    {
        return {make_buffer_core(size * sizeof(T)), size};
    }

    template<typename T>
    message_buffer<T> make_buffer(T* ptr, std::size_t size)
    {
        return {make_buffer_core(ptr, size * sizeof(T)), size};
    }

#if OOMPH_ENABLE_DEVICE
    template<typename T>
    message_buffer<T> make_device_buffer(std::size_t size, int id = hwmalloc::get_device_id())
    {
        return {make_buffer_core(size * sizeof(T), id), size};
    }

    template<typename T>
    message_buffer<T> make_device_buffer(T* device_ptr, std::size_t size,
        int id = hwmalloc::get_device_id())
    {
        return {make_buffer_core(device_ptr, size * sizeof(T), id), size};
    }

    template<typename T>
    message_buffer<T> make_device_buffer(T* ptr, T* device_ptr, std::size_t size,
        int id = hwmalloc::get_device_id())
    {
        return {make_buffer_core(ptr, device_ptr, size * sizeof(T), id), size};
    }
#endif

    // no callback versions
    // ====================

    // recv
    // ----

    template<typename T>
    recv_request recv(message_buffer<T>& msg, rank_type src, tag_type tag)
    {
        assert(msg);
        return recv(msg.m.m_heap_ptr.get(), msg.size() * sizeof(T), src, tag,
            util::unique_function<void(rank_type, tag_type)>([](rank_type, tag_type) {}));
    }

    // shared_recv
    // -----------

    template<typename T>
    shared_recv_request shared_recv(message_buffer<T>& msg, rank_type src, tag_type tag)
    {
        assert(msg);
        return shared_recv(msg.m.m_heap_ptr.get(), msg.size() * sizeof(T), src, tag,
            util::unique_function<void(rank_type, tag_type)>([](rank_type, tag_type) {}));
    }

    // send
    // ----

    template<typename T>
    send_request send(message_buffer<T> const& msg, rank_type dst, tag_type tag)
    {
        assert(msg);
        return send(msg.m.m_heap_ptr.get(), msg.size() * sizeof(T), dst, tag,
            util::unique_function<void(rank_type, tag_type)>([](rank_type, tag_type) {}));
    }

    // send_multi
    // ----------

    template<typename T>
    send_multi_request send_multi(message_buffer<T> const& msg, rank_type const* neighs,
        std::size_t neighs_size, tag_type tag)
    {
        assert(msg);
        auto mrs = m_state->make_multi_request_state(neighs_size);
        for (std::size_t i = 0; i < neighs_size; ++i)
        {
            send(msg.m.m_heap_ptr.get(), msg.size() * sizeof(T), neighs[i], tag,
                util::unique_function<void(rank_type, tag_type)>(
                    [mrs](rank_type, tag_type) { --(mrs->m_counter); }));
        }
        return {std::move(mrs)};
    }

    template<typename T>
    send_multi_request send_multi(message_buffer<T> const& msg,
        std::vector<rank_type> const& neighs, tag_type tag)
    {
        return send_multi(msg, neighs.data(), neighs.size(), tag);
    }

    template<typename T>
    send_multi_request send_multi(message_buffer<T> const& msg, rank_type const* neighs,
        tag_type const* tags, std::size_t neighs_size)
    {
        assert(msg);
        auto mrs = m_state->make_multi_request_state(neighs_size);
        for (std::size_t i = 0; i < neighs_size; ++i)
        {
            send(msg.m.m_heap_ptr.get(), msg.size() * sizeof(T), neighs[i], tags[i],
                util::unique_function<void(rank_type, tag_type)>(
                    [mrs](rank_type, tag_type) { --(mrs->m_counter); }));
        }
        return {std::move(mrs)};
    }

    template<typename T>
    send_multi_request send_multi(message_buffer<T> const& msg,
        std::vector<rank_type> const& neighs, std::vector<tag_type> const& tags)
    {
        assert(neighs.size() == tags.size());
        return send_multi(msg, neighs.data(), tags.data(), neighs.size());
    }

    // callback versions
    // =================

    // recv
    // ----

    template<typename T, typename CallBack>
    recv_request recv(message_buffer<T>&& msg, rank_type src, tag_type tag, CallBack&& callback)
    {
        OOMPH_CHECK_CALLBACK(CallBack)
        assert(msg);
        const auto s = msg.size();
        auto       m_ptr = msg.m.m_heap_ptr.get();
        return recv(m_ptr, s * sizeof(T), src, tag,
            util::unique_function<void(rank_type, tag_type)>(
                cb_rref<T, CallBack>{std::forward<CallBack>(callback), std::move(msg)}));
    }

    template<typename T, typename CallBack>
    recv_request recv(message_buffer<T>& msg, rank_type src, tag_type tag, CallBack&& callback)
    {
        OOMPH_CHECK_CALLBACK_REF(CallBack)
        assert(msg);
        const auto s = msg.size();
        auto       m_ptr = msg.m.m_heap_ptr.get();
        return recv(m_ptr, s * sizeof(T), src, tag,
            util::unique_function<void(rank_type, tag_type)>(
                cb_lref<T, CallBack>{std::forward<CallBack>(callback), &msg}));
    }

    // shared_recv
    // -----------

    template<typename T, typename CallBack>
    shared_recv_request shared_recv(message_buffer<T>&& msg, rank_type src, tag_type tag,
        CallBack&& callback)
    {
        OOMPH_CHECK_CALLBACK(CallBack)
        assert(msg);
        const auto s = msg.size();
        auto       m_ptr = msg.m.m_heap_ptr.get();
        return shared_recv(m_ptr, s * sizeof(T), src, tag,
            util::unique_function<void(rank_type, tag_type)>(
                cb_rref<T, CallBack>{std::forward<CallBack>(callback), std::move(msg)}));
    }

    template<typename T, typename CallBack>
    shared_recv_request shared_recv(message_buffer<T>& msg, rank_type src, tag_type tag,
        CallBack&& callback)
    {
        OOMPH_CHECK_CALLBACK_REF(CallBack)
        assert(msg);
        const auto s = msg.size();
        auto       m_ptr = msg.m.m_heap_ptr.get();
        return shared_recv(m_ptr, s * sizeof(T), src, tag,
            util::unique_function<void(rank_type, tag_type)>(
                cb_lref<T, CallBack>{std::forward<CallBack>(callback), &msg}));
    }

    // send
    // ----

    template<typename T, typename CallBack>
    send_request send(message_buffer<T>&& msg, rank_type dst, tag_type tag, CallBack&& callback)
    {
        OOMPH_CHECK_CALLBACK(CallBack)
        assert(msg);
        const auto s = msg.size();
        auto       m_ptr = msg.m.m_heap_ptr.get();
        return send(m_ptr, s * sizeof(T), dst, tag,
            util::unique_function<void(rank_type, tag_type)>(
                cb_rref<T, CallBack>{std::forward<CallBack>(callback), std::move(msg)}));
    }

    template<typename T, typename CallBack>
    send_request send(message_buffer<T>& msg, rank_type dst, tag_type tag, CallBack&& callback)
    {
        OOMPH_CHECK_CALLBACK_REF(CallBack)
        assert(msg);
        const auto s = msg.size();
        auto       m_ptr = msg.m.m_heap_ptr.get();
        return send(m_ptr, s * sizeof(T), dst, tag,
            util::unique_function<void(rank_type, tag_type)>(
                cb_lref<T, CallBack>{std::forward<CallBack>(callback), &msg}));
    }

    template<typename T, typename CallBack>
    send_request send(message_buffer<T> const& msg, rank_type dst, tag_type tag,
        CallBack&& callback)
    {
        OOMPH_CHECK_CALLBACK_CONST_REF(CallBack)
        assert(msg);
        const auto s = msg.size();
        auto       m_ptr = msg.m.m_heap_ptr.get();
        return send(m_ptr, s * sizeof(T), dst, tag,
            util::unique_function<void(rank_type, tag_type)>(
                cb_lref_const<T, CallBack>{std::forward<CallBack>(callback), &msg}));
    }

    // send_multi
    // ----------

    template<typename T, typename CallBack>
    send_multi_request send_multi(message_buffer<T>&& msg, std::vector<rank_type> neighs,
        tag_type tag, CallBack&& callback)
    {
        OOMPH_CHECK_CALLBACK_MULTI(CallBack)
        assert(msg);
        auto const s = msg.size();
        auto       m_ptr = msg.m.m_heap_ptr.get();
        auto       mrs = m_state->make_multi_request_state(std::move(neighs), std::move(msg));
        for (auto dst : mrs->m_neighs)
        {
            send(m_ptr, s * sizeof(T), dst, tag,
                util::unique_function<void(rank_type, tag_type)>(
                    [mrs, callback](rank_type, tag_type t)
                    {
                        if (--(mrs->m_counter) == 0ul)
                        {
                            callback(message_buffer<T>(std::move(mrs->m_msg), mrs->m_msg_size),
                                std::move(mrs->m_neighs), t);
                        }
                    }));
        }
        return {std::move(mrs)};
    }

    template<typename T, typename CallBack>
    send_multi_request send_multi(message_buffer<T>&& msg, std::vector<rank_type> neighs,
        std::vector<tag_type> tags, CallBack&& callback)
    {
        OOMPH_CHECK_CALLBACK_MULTI_TAGS(CallBack)
        assert(msg);
        assert(neighs.size() == tags.size());
        auto const s = msg.size();
        auto       m_ptr = msg.m.m_heap_ptr.get();
        auto       mrs =
            m_state->make_multi_request_state(std::move(neighs), std::move(tags), std::move(msg));
        const auto n = mrs->m_neighs.size();
        for (std::size_t i = 0; i < n; ++i)
        {
            send(m_ptr, s * sizeof(T), mrs->m_neighs[i], mrs->m_tags[i],
                util::unique_function<void(rank_type, tag_type)>(
                    [mrs, callback](rank_type, tag_type)
                    {
                        if (--(mrs->m_counter) == 0ul)
                        {
                            callback(message_buffer<T>(std::move(mrs->m_msg), mrs->m_msg_size),
                                std::move(mrs->m_neighs), mrs->m_tags);
                        }
                    }));
        }
        return {std::move(mrs)};
    }

    template<typename T, typename CallBack>
    send_multi_request send_multi(message_buffer<T>& msg, std::vector<rank_type> neighs,
        tag_type tag, CallBack&& callback)
    {
        OOMPH_CHECK_CALLBACK_MULTI_REF(CallBack)
        assert(msg);
        auto const s = msg.size();
        auto       m_ptr = msg.m.m_heap_ptr.get();
        auto       mrs = m_state->make_multi_request_state(std::move(neighs), msg);
        for (auto dst : mrs->m_neighs)
        {
            send(m_ptr, s * sizeof(T), dst, tag,
                util::unique_function<void(rank_type, tag_type)>(
                    [mrs, callback](rank_type, tag_type t)
                    {
                        if (--(mrs->m_counter) == 0ul)
                        {
                            callback(*reinterpret_cast<message_buffer<T>*>(mrs->m_msg_ptr),
                                std::move(mrs->m_neighs), t);
                        }
                    }));
        }
        return {std::move(mrs)};
    }

    template<typename T, typename CallBack>
    send_multi_request send_multi(message_buffer<T>& msg, std::vector<rank_type> neighs,
        std::vector<tag_type> tags, CallBack&& callback)
    {
        OOMPH_CHECK_CALLBACK_MULTI_REF_TAGS(CallBack)
        assert(msg);
        assert(neighs.size() == tags.size());
        auto const s = msg.size();
        auto       m_ptr = msg.m.m_heap_ptr.get();
        auto       mrs = m_state->make_multi_request_state(std::move(neighs), std::move(tags), msg);
        const auto n = mrs->m_neighs.size();
        for (std::size_t i = 0; i < n; ++i)
        {
            send(m_ptr, s * sizeof(T), mrs->m_neighs[i], mrs->m_tags[i],
                util::unique_function<void(rank_type, tag_type)>(
                    [mrs, callback](rank_type, tag_type)
                    {
                        if (--(mrs->m_counter) == 0ul)
                        {
                            callback(*reinterpret_cast<message_buffer<T>*>(mrs->m_msg_ptr),
                                std::move(mrs->m_neighs), std::move(mrs->m_tags));
                        }
                    }));
        }
        return {std::move(mrs)};
    }

    template<typename T, typename CallBack>
    send_multi_request send_multi(message_buffer<T> const& msg, std::vector<rank_type> neighs,
        tag_type tag, CallBack&& callback)
    {
        OOMPH_CHECK_CALLBACK_MULTI_CONST_REF(CallBack)
        assert(msg);
        auto const s = msg.size();
        auto       m_ptr = msg.m.m_heap_ptr.get();
        auto       mrs = m_state->make_multi_request_state(std::move(neighs), msg);
        for (auto dst : mrs->m_neighs)
        {
            send(m_ptr, s * sizeof(T), dst, tag,
                util::unique_function<void(rank_type, tag_type)>(
                    [mrs, callback](rank_type, tag_type t)
                    {
                        if (--(mrs->m_counter) == 0ul)
                        {
                            callback(*reinterpret_cast<message_buffer<T> const*>(mrs->m_msg_ptr),
                                std::move(mrs->m_neighs), t);
                        }
                    }));
        }
        return {std::move(mrs)};
    }

    template<typename T, typename CallBack>
    send_multi_request send_multi(message_buffer<T> const& msg, std::vector<rank_type> neighs,
        std::vector<tag_type> tags, CallBack&& callback)
    {
        OOMPH_CHECK_CALLBACK_MULTI_CONST_REF_TAGS(CallBack)
        assert(msg);
        assert(neighs.size() == tags.size());
        auto const s = msg.size();
        auto       m_ptr = msg.m.m_heap_ptr.get();
        auto       mrs = m_state->make_multi_request_state(std::move(neighs), std::move(tags), msg);
        const auto n = mrs->m_neighs.size();
        for (std::size_t i = 0; i < n; ++i)
        {
            send(m_ptr, s * sizeof(T), mrs->m_neighs[i], mrs->m_tags[i],
                util::unique_function<void(rank_type, tag_type)>(
                    [mrs, callback](rank_type, tag_type)
                    {
                        if (--(mrs->m_counter) == 0ul)
                        {
                            callback(*reinterpret_cast<message_buffer<T> const*>(mrs->m_msg_ptr),
                                std::move(mrs->m_neighs), std::move(mrs->m_tags));
                        }
                    }));
        }
        return {std::move(mrs)};
    }

    void progress();

  private:
    detail::message_buffer make_buffer_core(std::size_t size);
    detail::message_buffer make_buffer_core(void* ptr, std::size_t size);
#if OOMPH_ENABLE_DEVICE
    detail::message_buffer make_buffer_core(std::size_t size, int device_id);
    detail::message_buffer make_buffer_core(void* device_ptr, std::size_t size, int device_id);
    detail::message_buffer make_buffer_core(void* ptr, void* device_ptr, std::size_t size,
        int device_id);
#endif

    send_request send(detail::message_buffer::heap_ptr_impl const* m_ptr, std::size_t size,
        rank_type dst, tag_type tag, util::unique_function<void(rank_type, tag_type)>&& cb);

    recv_request recv(detail::message_buffer::heap_ptr_impl* m_ptr, std::size_t size, rank_type src,
        tag_type tag, util::unique_function<void(rank_type, tag_type)>&& cb);

    shared_recv_request shared_recv(detail::message_buffer::heap_ptr_impl* m_ptr, std::size_t size,
        rank_type src, tag_type tag, util::unique_function<void(rank_type, tag_type)>&& cb);
};

} // namespace oomph
