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

#include <cstdint>
#include <stack>
#include <type_traits>

#include <boost/lockfree/queue.hpp>

#include <oomph/communicator.hpp>
#include <oomph/context.hpp>

// paths relative to backend
#include <../communicator_base.hpp>
#include <../device_guard.hpp>
#include <context.hpp>
#include <controller.hpp>
#include <operation_context.hpp>
#include <request_state.hpp>

namespace oomph
{

MAKE_LOGGER(comm_log, "OomphCom")

using operation_context = libfabric::operation_context;

class communicator_impl : public communicator_base<communicator_impl>
{
    using tag_type = std::uint64_t;
    //
    using segment_type = libfatbat::memory_segment;
    using region_type = segment_type::handle_type;

    using callback_queue = boost::lockfree::queue<detail::request_state*,
        boost::lockfree::fixed_sized<false>, boost::lockfree::allocator<std::allocator<void>>>;

  public:
    context_impl*               m_context;
    libfatbat::endpoint_wrapper m_tx_endpoint;
    libfatbat::endpoint_wrapper m_rx_endpoint;
    //
    callback_queue m_send_cb_queue;
    callback_queue m_recv_cb_queue;
    callback_queue m_recv_cb_cancel;

    template<typename T, bool IsPointer = std::is_pointer<typename std::decay<T>::type>::value,
        bool IsIntegral = std::is_integral<typename std::decay<T>::type>::value>
    struct mpi_format_helper
    {
        static char const* cast(T const&) { return "[opaque]"; }
    };

    template<typename T>
    struct mpi_format_helper<T, true, false>
    {
        static void const* cast(T value) { return static_cast<void const*>(value); }
    };

    template<typename T>
    struct mpi_format_helper<T, false, true>
    {
        static unsigned long long cast(T value) { return static_cast<unsigned long long>(value); }
    };

    template<typename T>
    static auto mpi_format(T value) -> decltype(mpi_format_helper<T>::cast(value))
    {
        return mpi_format_helper<T>::cast(value);
    }

    // --------------------------------------------------------------------
    communicator_impl(context_impl* ctxt)
    : communicator_base(ctxt)
    , m_context(ctxt)
    , m_send_cb_queue(128)
    , m_recv_cb_queue(128)
    , m_recv_cb_cancel(8)
    {
        LIBFATBAT_DEBUG(comm_log, "{:<20} MPI_comm {} ", "Construct",
            mpi_format_helper<decltype(mpi_comm())>::cast(mpi_comm()));
        m_tx_endpoint = m_context->get_controller()->get_tx_endpoint();
        m_rx_endpoint = m_context->get_controller()->get_rx_endpoint();
    }

    // --------------------------------------------------------------------
    ~communicator_impl() { clear_callback_queues(); }

    // --------------------------------------------------------------------
    auto& get_heap() noexcept { return m_context->get_heap(); }

    // --------------------------------------------------------------------
    /// generate a tag with 0xRRRRRRRRtttttttt rank, tag.
    /// original tag can be 32bits, then we add 32bits of rank info.
    inline std::uint64_t make_tag64(std::uint32_t tag, /*std::uint32_t rank, */ std::uintptr_t ctxt)
    {
        return (((ctxt & 0x0000'0000'00FF'FFFF) << 24) |
                ((std::uint64_t(tag) & 0x0000'0000'00FF'FFFF)));
    }

    // --------------------------------------------------------------------
    template<typename Func, typename... Args>
    inline void execute_fi_function(Func F, char const* msg, Args&&... args)
    {
        bool ok = false;
        while (!ok)
        {
            ssize_t ret = F(std::forward<Args>(args)...);
            if (ret == 0) { return; }
            else if (ret == -FI_EAGAIN)
            {
                // com_deb<9>.error("Reposting", msg);
                // no point stressing the system
                m_context->get_controller()->poll_for_work_completions(this);
            }
            else if (ret == -FI_ENOENT)
            {
                // if a node has failed, we can recover
                // @TODO : put something better here
                LIBFATBAT_ERROR(comm_log, "{:<20} No destination endpoint, terminating.",
                    "fi_function");
                std::terminate();
            }
            else if (ret) { throw libfatbat::fabric_error(int(ret), msg); }
        }
    }

    // --------------------------------------------------------------------
    // this takes a pinned memory region and sends it
    void send_tagged_region(region_type const& send_region, std::size_t size, fi_addr_t dst_addr_,
        uint64_t tag_, operation_context* ctxt)
    {
        LIBFATBAT_SCOPE(comm_log, "{} {}", (void*)(this), __func__);
        LIBFATBAT_DEBUG(comm_log, "{:<20} -> {:02} {} tag {:#12x} context {} tx endpoint {}",
            "send_tagged_region", dst_addr_, send_region, tag_, static_cast<void*>(ctxt),
            static_cast<void*>(m_tx_endpoint.get_ep()));
        execute_fi_function(fi_tsend, "fi_tsend", m_tx_endpoint.get_ep(), send_region.get_address(),
            size, send_region.get_local_key(), dst_addr_, tag_, ctxt);
    }

    // --------------------------------------------------------------------
    // this takes a pinned memory region and sends it using inject instead of send
    void inject_tagged_region(region_type const& send_region, std::size_t size, fi_addr_t dst_addr_,
        uint64_t tag_)
    {
        LIBFATBAT_SCOPE(comm_log, "{} {}", (void*)(this), __func__);
        LIBFATBAT_DEBUG(comm_log, "{:<20} -> {:02} {} tag {} tx endpoint {}",
            "inject_tagged_region", dst_addr_, send_region, tag_,
            static_cast<void*>(m_tx_endpoint.get_ep()));
        execute_fi_function(fi_tinject, "fi_tinject", m_tx_endpoint.get_ep(),
            send_region.get_address(), size, dst_addr_, tag_);
    }

    // --------------------------------------------------------------------
    // the receiver posts a single receive buffer to the queue, attaching
    // itself as the context, so that when a message is received
    // the owning receiver is called to handle processing of the buffer
    void recv_tagged_region(region_type const& recv_region, std::size_t size, fi_addr_t src_addr_,
        uint64_t tag_, operation_context* ctxt)
    {
        LIBFATBAT_SCOPE(comm_log, "{} {}", (void*)(this), __func__);
        LIBFATBAT_DEBUG(comm_log, "{:<20} <- {:02} {} tag {} context {} rx endpoint {}",
            "recv_tagged_region", src_addr_, recv_region, tag_, static_cast<void*>(ctxt),
            static_cast<void*>(m_rx_endpoint.get_ep()));
        constexpr uint64_t ignore = 0;
        execute_fi_function(fi_trecv, "fi_trecv", m_rx_endpoint.get_ep(), recv_region.get_address(),
            size, recv_region.get_local_key(), src_addr_, tag_, ignore, ctxt);
        // if (l.owns_lock()) l.unlock();
    }

    // --------------------------------------------------------------------
    send_request send(context_impl::heap_type::pointer const& ptr, std::size_t size, rank_type dst,
        oomph::tag_type tag, util::unique_function<void(rank_type, oomph::tag_type)>&& cb,
        std::size_t* scheduled)
    {
        LIBFATBAT_SCOPE(comm_log, "{} {}", (void*)(this), __func__);
        std::uint64_t stag = make_tag64(tag, /*this->rank(), */ this->m_context->get_context_tag());

#if OOMPH_ENABLE_DEVICE
        auto const& reg = ptr.on_device() ? ptr.device_handle() : ptr.handle();
#else
        auto const& reg = ptr.handle();
#endif

#ifdef EXTRA_SIZE_CHECKS
        if (size != reg.get_size())
        {
            LIBFATBAT_ERROR(comm_log, "{:<20} size {:#06x} reg size {:#06x} send mismatch", "send",
                size, reg.get_size());
        }
#endif
        m_context->get_controller()->sends_posted_++;

        // use optimized inject if msg is very small
        if (size <= m_context->get_controller()->get_tx_inject_size())
        {
            inject_tagged_region(reg, size, fi_addr_t(dst), stag);
            if (!has_reached_recursion_depth())
            {
                auto inc = recursion();
                cb(dst, tag);
                return {};
            }
            else
            {
                // construct request which is also an operation context
                auto s =
                    m_req_state_factory.make(m_context, this, scheduled, dst, tag, std::move(cb));
                s->create_self_ref();
                while (!m_send_cb_queue.push(s.get())) {}
                return {std::move(s)};
            }
        }

        // construct request which is also an operation context
        auto s = m_req_state_factory.make(m_context, this, scheduled, dst, tag, std::move(cb));
        s->create_self_ref();

        LIBFATBAT_DEBUG(comm_log,
            "{:<20} thisrank {} rank {} tag {:#12x} stag {:#12x} addr {} size {:#06x} reg size {:#06x} op_ctx {} req {}",
            "send", rank(), dst, std::uint64_t(tag), stag, static_cast<void*>(reg.get_address()),
            size, reg.get_size(), static_cast<void*>(&(s->m_operation_context)),
            static_cast<void*>(s.get()));
#if OOMPH_ENABLE_DEVICE
        if (!ptr.on_device())
        {
            LIBFATBAT_DEBUG(comm_log, "{:<20} {}", "send device region",
                libfatbat::log::mem_crc32(reg.get_address(), size));
        }
#endif

        send_tagged_region(reg, size, fi_addr_t(dst), stag, &(s->m_operation_context));
        return {std::move(s)};
    }

    recv_request recv(context_impl::heap_type::pointer& ptr, std::size_t size, rank_type src,
        oomph::tag_type tag, util::unique_function<void(rank_type, oomph::tag_type)>&& cb,
        std::size_t* scheduled)
    {
        LIBFATBAT_SCOPE(comm_log, "{} {}", (void*)(this), __func__);
        std::uint64_t stag = make_tag64(tag, /*src, */ this->m_context->get_context_tag());

#if OOMPH_ENABLE_DEVICE
        auto const& reg = ptr.on_device() ? ptr.device_handle() : ptr.handle();
#else
        auto const& reg = ptr.handle();
#endif

#ifdef EXTRA_SIZE_CHECKS
        if (size != reg.get_size())
        {
            LIBFATBAT_ERROR(comm_log, "{:<20} size {:#06x} reg size {:#06x} recv mismatch", "recv",
                size, reg.get_size());
        }
#endif
        m_context->get_controller()->recvs_posted_++;

        // construct request which is also an operation context
        auto s = m_req_state_factory.make(m_context, this, scheduled, src, tag, std::move(cb));
        s->create_self_ref();

        LIBFATBAT_DEBUG(comm_log,
            "{:<20} thisrank {} rank {} tag {:#12x} stag {:#12x} addr {} size {:#06x} reg size {:#06x} op_ctx {} req {}",
            "recv", rank(), src, std::uint64_t(tag), stag, static_cast<void*>(reg.get_address()),
            size, reg.get_size(), static_cast<void*>(&(s->m_operation_context)),
            static_cast<void*>(s.get()));
#if OOMPH_ENABLE_DEVICE
        if (!ptr.on_device())
        {
            LIBFATBAT_DEBUG(comm_log, "{:<20} {}", "recv device region",
                libfatbat::log::mem_crc32(reg.get_address(), size));
        }
#endif

        recv_tagged_region(reg, size, fi_addr_t(src), stag, &(s->m_operation_context));
        return {std::move(s)};
    }

    shared_recv_request shared_recv(context_impl::heap_type::pointer& ptr, std::size_t size,
        rank_type src, oomph::tag_type tag,
        util::unique_function<void(rank_type, oomph::tag_type)>&& cb,
        std::atomic<std::size_t>*                                 scheduled)
    {
        LIBFATBAT_SCOPE(comm_log, "{} {}", (void*)(this), __func__);
        std::uint64_t stag = make_tag64(tag, /*src, */ this->m_context->get_context_tag());

#if OOMPH_ENABLE_DEVICE
        auto const& reg = ptr.on_device() ? ptr.device_handle() : ptr.handle();
#else
        auto const& reg = ptr.handle();
#endif

#ifdef EXTRA_SIZE_CHECKS
        if (size != reg.get_size())
        {
            LIBFATBAT_ERROR(comm_log, "{:<20} size {:#06x} reg size {:#06x} recv mismatch", "recv",
                size, reg.get_size());
        }
#endif
        m_context->get_controller()->recvs_posted_++;

        // construct request which is also an operation context
        auto s = std::make_shared<detail::shared_request_state>(m_context, this, scheduled, src,
            tag, std::move(cb));
        s->create_self_ref();

        LIBFATBAT_DEBUG(comm_log,
            "{:<20} thisrank {} rank {} tag {:#12x} stag {:#12x} addr {} size {:#06x} reg size {:#06x} op_ctx {} req {}",
            "shared_recv", rank(), src, std::uint64_t(tag), stag,
            static_cast<void*>(reg.get_address()), size, reg.get_size(),
            static_cast<void*>(&(s->m_operation_context)), static_cast<void*>(s.get()));

        recv_tagged_region(reg, size, fi_addr_t(src), stag, &(s->m_operation_context));
        m_context->get_controller()->poll_recv_queue(m_rx_endpoint.get_rx_cq(), this);
        return {std::move(s)};
    }

    void progress()
    {
        m_context->get_controller()->poll_for_work_completions(this);
        clear_callback_queues();
    }

    void clear_callback_queues()
    {
        // work through ready callbacks, which were pushed to the queue
        // (by other threads)
        m_send_cb_queue.consume_all(
            [](oomph::detail::request_state* req)
            {
                LIBFATBAT_SCOPE(comm_log, "{:<20} req {} callback", "m_send_cb_queue.consume_all",
                    static_cast<void*>(req));
                auto ptr = req->release_self_ref();
                req->invoke_cb();
            });

        m_recv_cb_queue.consume_all(
            [](oomph::detail::request_state* req)
            {
                LIBFATBAT_SCOPE(comm_log, "{:<20} req {} callback", "m_recv_cb_queue.consume_all",
                    static_cast<void*>(req));
                auto ptr = req->release_self_ref();
                req->invoke_cb();
            });
        m_context->m_recv_cb_queue.consume_all(
            [](detail::shared_request_state* req)
            {
                auto ptr = req->release_self_ref();
                req->invoke_cb();
            });
    }

    // Cancel is a problem with libfabric because fi_cancel is asynchronous.
    // The item to be cancelled will either complete with CANCELLED status
    // or will complete as usual (ie before the cancel could take effect)
    //
    // We can only be certain if we poll until the completion happens
    // or attach a callback to the cancel notification which is not supported
    // by oomph.
    bool cancel_recv(detail::request_state* s)
    {
        // get the original message operation context
        operation_context* op_ctx = &(s->m_operation_context);

        // submit the cancellation request
        bool ok = (fi_cancel(&m_rx_endpoint.get_ep()->fid, op_ctx) == 0);
        LIBFATBAT_DEBUG(comm_log, "{:<20} op_ctx {} fi_cancel ok {}", "cancel_recv",
            static_cast<void*>(op_ctx), ok);

        // if the cancel operation failed completely, return
        if (!ok) return false;

        bool found = false;
        while (!found)
        {
            m_context->get_controller()->poll_recv_queue(m_rx_endpoint.get_rx_cq(), this);
            // otherwise, poll until we know if it worked
            std::stack<detail::request_state*> temp_stack;
            detail::request_state*             temp;
            while (!found && m_recv_cb_cancel.pop(temp))
            {
                if (temp == s)
                {
                    // our recv was cancelled correctly
                    found = true;
                    LIBFATBAT_DEBUG(comm_log, "{:<20} op_ctx {} fi_cancel ok {}", "cancel_recv",
                        static_cast<void*>(op_ctx), ok);
                    auto ptr = s->release_self_ref();
                    s->set_canceled();
                }
                else
                {
                    // a different cancel operation
                    temp_stack.push(temp);
                }
            }
            // return any weird unhandled cancels back to the queue
            while (!temp_stack.empty())
            {
                auto temp = temp_stack.top();
                temp_stack.pop();
                m_recv_cb_cancel.push(temp);
            }
        }
        return found;
    }
};

} // namespace oomph
