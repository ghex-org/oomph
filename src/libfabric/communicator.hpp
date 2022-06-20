/*
 * GridTools
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#pragma once

#include <cstdint>
#include <stack>

#include <boost/lockfree/queue.hpp>

#include <oomph/context.hpp>
#include <oomph/communicator.hpp>
//
#include "../communicator_base.hpp"
#include "../device_guard.hpp"
//
#include "./operation_context.hpp"
#include "./request_state.hpp"
#include "./controller.hpp"
#include "./context.hpp"

namespace oomph
{

using operation_context = oomph::libfabric::operation_context;

using tag_disp = oomph::debug::detail::hex<12, uintptr_t>;

// cppcheck-suppress ConfigurationNotChecked
static NS_DEBUG::enable_print<false> com_deb("COMMUNI");
static NS_DEBUG::enable_print<true>  com_err("COMMUNI");

class communicator_impl : public communicator_base<communicator_impl>
{
    using rank_type = communicator::rank_type;
    using tag_type = std::uint64_t;
    //
    using segment_type = libfabric::memory_segment;
    using region_type = segment_type::handle_type;

    using callback_queue = boost::lockfree::queue<detail::request_state*,
        boost::lockfree::fixed_sized<false>, boost::lockfree::allocator<std::allocator<void>>>;

  public:
    context_impl*               m_context;
    std::uintptr_t              m_ctag;
    libfabric::endpoint_wrapper m_tx_endpoint;
    libfabric::endpoint_wrapper m_rx_endpoint;
    //
    callback_queue m_send_cb_queue;
    callback_queue m_recv_cb_queue;
    callback_queue m_recv_cb_cancel;

    // --------------------------------------------------------------------
    communicator_impl(context_impl* ctxt)
    : communicator_base(ctxt)
    , m_context(ctxt)
    , m_ctag(0)
    , m_send_cb_queue(128)
    , m_recv_cb_queue(128)
    , m_recv_cb_cancel(8)
    {
        OOMPH_DP_ONLY(com_deb, debug(NS_DEBUG::str<>("MPI_comm"), NS_DEBUG::ptr(mpi_comm())));
        m_tx_endpoint = m_context->get_controller()->get_tx_endpoint();
        m_rx_endpoint = m_context->get_controller()->get_rx_endpoint();

        // seems like we don't need this any more, I don't want to delete it yet ...
        // this chunk of code might be needed if the same tag is used
        // simultaneously on differnt communicators
#ifdef ADD_COMM_ID_TO_TAG
        const int random_msg_tag = 65535;
        if (rank() == 0)
        {
            m_ctag = reinterpret_cast<std::uintptr_t>(this);
            OOMPH_DP_ONLY(com_deb, debug(NS_DEBUG::str<>("MPI send tag"), tag_disp(m_ctag)));
            for (int i = 1; i < size(); ++i)
            {
                MPI_Send(&m_ctag, sizeof(std::uintptr_t), MPI_CHAR, i, random_msg_tag, mpi_comm());
            }
        }
        else
        {
            MPI_Status status;
            MPI_Recv(&m_ctag, sizeof(std::uintptr_t), MPI_CHAR, 0, random_msg_tag, mpi_comm(),
                &status);
            OOMPH_DP_ONLY(com_deb, debug(NS_DEBUG::str<>("MPI recv tag"), tag_disp(m_ctag)));
        }
#endif
    }

    // --------------------------------------------------------------------
    ~communicator_impl() { clear_callback_queues(); }

    // --------------------------------------------------------------------
    auto& get_heap() noexcept { return m_context->get_heap(); }

    // --------------------------------------------------------------------
    /// generate a tag with 0xaaaaaaRRRRtttttt commmunicator  address, rank, tag.
    /// original tag can be 24bits, then we add 16bits of rank info,
    /// and 24bits of commmunicator address (currently unset).
    /// if different MPI communicators were used and shared tag values/ranges, then
    /// we would need to add back the oomph communicator address as a way
    /// of discriminating diffferent messages with the same tag
    inline std::uint64_t make_tag64(std::uint32_t tag, std::uint32_t rank)
    {
        return (((std::uint64_t(m_ctag) & 0x0000000000FFFFFF) << 40) |
                ((std::uint64_t(rank) & 0x000000000000FFFF) << 24) |
                ((std::uint64_t(tag) & 0x0000000000FFFFFF)));
    }

    // --------------------------------------------------------------------
    template<typename Func, typename... Args>
    inline void execute_fi_function(Func F, const char* msg, Args&&... args)
    {
        bool ok = false;
        while (!ok)
        {
            ssize_t ret = F(std::forward<Args>(args)...);
            if (ret == 0) { return; }
            else if (ret == -FI_EAGAIN)
            {
                com_deb.error("Reposting", msg);
                // no point stressing the system
                m_context->get_controller()->poll_for_work_completions(this);
            }
            else if (ret == -FI_ENOENT)
            {
                // if a node has failed, we can recover
                // @TODO : put something better here
                com_err.error("No destination endpoint, terminating.");
                std::terminate();
            }
            else if (ret) { throw libfabric::fabric_error(int(ret), msg); }
        }
    }

    // --------------------------------------------------------------------
    // this takes a pinned memory region and sends it
    void send_tagged_region(region_type const& send_region, std::size_t size, fi_addr_t dst_addr_,
        uint64_t tag_, operation_context* ctxt)
    {
        [[maybe_unused]] auto scp = com_deb.scope(NS_DEBUG::ptr(this), __func__);
        OOMPH_DP_ONLY(com_deb,
            debug(NS_DEBUG::str<>("send message buffer"), "->", NS_DEBUG::dec<2>(dst_addr_),
                send_region, "tag", tag_disp(tag_), "context", NS_DEBUG::ptr(ctxt), "endpoint",
                NS_DEBUG::ptr(m_tx_endpoint.get_ep())));

        execute_fi_function(fi_tsend, "fi_tsend", m_tx_endpoint.get_ep(), send_region.get_address(),
            size, send_region.get_local_key(), dst_addr_, tag_, ctxt);
    }

    // --------------------------------------------------------------------
    // this takes a pinned memory region and sends it using inject instead of send
    void inject_tagged_region(region_type const& send_region, std::size_t size, fi_addr_t dst_addr_,
        uint64_t tag_)
    {
        [[maybe_unused]] auto scp = com_deb.scope(NS_DEBUG::ptr(this), __func__);
        OOMPH_DP_ONLY(com_deb,
            debug(NS_DEBUG::str<>("inject tagged"), "->", NS_DEBUG::dec<2>(dst_addr_), send_region,
                "tag", tag_disp(tag_), "endpoint", NS_DEBUG::ptr(m_tx_endpoint.get_ep())));
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
        [[maybe_unused]] auto scp = com_deb.scope(NS_DEBUG::ptr(this), __func__);

        OOMPH_DP_ONLY(com_deb,
            debug(NS_DEBUG::str<>("recv message buffer"), "<-", NS_DEBUG::dec<2>(src_addr_),
                recv_region, "tag", tag_disp(tag_), "context", NS_DEBUG::ptr(ctxt), "rx endpoint",
                NS_DEBUG::ptr(m_rx_endpoint.get_ep())));

        // auto l = std::move(m_context->get_controller()->get_rx_lock());
        constexpr uint64_t ignore = 0;
        execute_fi_function(fi_trecv, "fi_trecv", m_rx_endpoint.get_ep(), recv_region.get_address(),
            size, recv_region.get_local_key(), src_addr_, tag_, ignore, ctxt);
        // if (l.owns_lock()) l.unlock();
    }

    // --------------------------------------------------------------------
    send_request send(context_impl::heap_type::pointer const& ptr, std::size_t size, rank_type dst,
        int tag, util::unique_function<void(rank_type, int)>&& cb, std::size_t* scheduled)
    {
        //[[maybe_unused]] auto scp = com_deb.scope(NS_DEBUG::ptr(this), __func__, "req",
        //    NS_DEBUG::ptr(&req), "ctx", NS_DEBUG::ptr(&req->reserved()->operation_context_));
        std::uint64_t         stag = make_tag64(tag, this->rank());

        auto& reg = ptr.handle_ref();
#ifdef EXTRA_SIZE_CHECKS
        if (size != reg.get_size())
        {
            OOMPH_DP_ONLY(com_err,
                error(NS_DEBUG::str<>("send mismatch"), "size", NS_DEBUG::hex<6>(size), "reg size",
                    NS_DEBUG::hex<6>(reg.get_size())));
        }
#endif
        m_context->get_controller()->sends_posted_++;

        // use optimized inject if msg is very small
        if (size <= m_context->get_controller()->get_tx_inject_size())
        {
            inject_tagged_region(reg, size, fi_addr_t(dst), stag);
            cb(dst, tag);
            return {};
        }
        
        // construct request which is also an operation context
        auto s = m_req_state_factory.make(m_context, this, scheduled, dst, tag, std::move(cb));
        s->m_self_ptr = s;

        // clang-format off
        OOMPH_DP_ONLY(com_deb,
            debug(NS_DEBUG::str<>("Send"),
                  "thisrank", NS_DEBUG::dec<>(rank()),
                  "rank", NS_DEBUG::dec<>(dst),
                  "tag", tag_disp(std::uint64_t(tag)),
                  "ctag", tag_disp(m_ctag),
                  "stag", tag_disp(stag),
                  "addr", NS_DEBUG::ptr(reg.get_address()),
                  "size", NS_DEBUG::hex<6>(size),
                  "reg size", NS_DEBUG::hex<6>(reg.get_size()),
                  //"op_ctx", NS_DEBUG::ptr(op_ctx)));
                  "req", NS_DEBUG::ptr(s.get())));
        // clang-format on

        send_tagged_region(reg, size, fi_addr_t(dst), stag, &(s->m_operation_context));
        m_context->get_controller()->poll_send_queue(m_tx_endpoint.get_tx_cq(), this);
        return {std::move(s)};
    }
//    void send(context_impl::heap_type::pointer const& ptr, std::size_t size, rank_type dst,
//        tag_type tag, util::unique_function<void()>&& cb, communicator::shared_request_ptr&& req)
//    {
//        [[maybe_unused]] auto scp = com_deb.scope(NS_DEBUG::ptr(this), __func__, "req",
//            NS_DEBUG::ptr(&req), "ctx", NS_DEBUG::ptr(&req->reserved()->operation_context_));
//        std::uint64_t         stag = make_tag64(tag, this->rank());
//
//        auto& reg = ptr.handle_ref();
//#ifdef EXTRA_SIZE_CHECKS
//        if (size != reg.get_size())
//        {
//            OOMPH_DP_ONLY(com_err,
//                error(NS_DEBUG::str<>("send mismatch"), "size", NS_DEBUG::hex<6>(size), "reg size",
//                    NS_DEBUG::hex<6>(reg.get_size())));
//        }
//#endif
//        m_context->get_controller()->sends_posted_++;
//
//        // use optimized inject if msg is very small
//        if (size <= m_context->get_controller()->get_tx_inject_size())
//        {
//            inject_tagged_region(reg, size, fi_addr_t(dst), stag);
//            cb();
//            return;
//        }
//
//        // construct operation context in space reserved in request object
//        operation_context::cb_ptr_t cb_ptr = std::move(cb).release();
//        operation_context*          op_ctx = new (&req->reserved()->operation_context_)
//            operation_context(cb_ptr, &m_send_cb_queue, nullptr);
//        assert(reinterpret_cast<void*>(op_ctx) ==
//               reinterpret_cast<void*>(&req->reserved()->operation_context_));
//
//        // clang-format off
//        OOMPH_DP_ONLY(com_deb,
//            debug(NS_DEBUG::str<>("Send"),
//                  "thisrank", NS_DEBUG::dec<>(rank()),
//                  "rank", NS_DEBUG::dec<>(dst),
//                  "tag", tag_disp(std::uint64_t(tag)),
//                  "ctag", tag_disp(m_ctag),
//                  "stag", tag_disp(stag),
//                  "addr", NS_DEBUG::ptr(reg.get_address()),
//                  "size", NS_DEBUG::hex<6>(size),
//                  "reg size", NS_DEBUG::hex<6>(reg.get_size()),
//                  "op_ctx", NS_DEBUG::ptr(op_ctx)));
//        // clang-format on
//
//        send_tagged_region(reg, size, fi_addr_t(dst), stag, op_ctx);
//        m_context->get_controller()->poll_send_queue(m_tx_endpoint.get_tx_cq());
//    }

    recv_request recv(context_impl::heap_type::pointer& ptr, std::size_t size, rank_type src,
        int tag, util::unique_function<void(rank_type, int)>&& cb, std::size_t* scheduled)
    {
        //[[maybe_unused]] auto scp = com_deb.scope(NS_DEBUG::ptr(this), __func__, "req",
        //    NS_DEBUG::ptr(&req), "ctx", NS_DEBUG::ptr(&req->reserved()->operation_context_));
        std::uint64_t         stag = make_tag64(tag, src);

        auto& reg = ptr.handle_ref();
#ifdef EXTRA_SIZE_CHECKS
        if (size != reg.get_size())
        {
            OOMPH_DP_ONLY(com_err,
                error(NS_DEBUG::str<>("recv mismatch"), "size", NS_DEBUG::hex<6>(size), "reg size",
                    NS_DEBUG::hex<6>(reg.get_size())));
        }
#endif
        m_context->get_controller()->recvs_posted_++;

        // construct request which is also an operation context
        auto s = m_req_state_factory.make(m_context, this, scheduled, src, tag, std::move(cb));
        s->m_self_ptr = s;

        // clang-format off
        OOMPH_DP_ONLY(com_deb,
            debug(NS_DEBUG::str<>("Recv"),
                  "thisrank", NS_DEBUG::dec<>(rank()),
                  "rank", NS_DEBUG::dec<>(src),
                  "tag", tag_disp(std::uint64_t(tag)),
                  "ctag", tag_disp(m_ctag),
                  "stag", tag_disp(stag),
                  "addr", NS_DEBUG::ptr(reg.get_address()),
                  "size", NS_DEBUG::hex<6>(size),
                  "reg size", NS_DEBUG::hex<6>(reg.get_size()),
                  //"op_ctx", NS_DEBUG::ptr(op_ctx)));
                  "req", NS_DEBUG::ptr(s.get())));
        // clang-format on

        recv_tagged_region(reg, size, fi_addr_t(src), stag, &(s->m_operation_context));
        m_context->get_controller()->poll_recv_queue(m_rx_endpoint.get_rx_cq(), this);
        return {std::move(s)};
    }
//    void recv(context_impl::heap_type::pointer& ptr, std::size_t size, rank_type src, tag_type tag,
//        util::unique_function<void()>&& cb, communicator::shared_request_ptr&& req)
//    {
//        [[maybe_unused]] auto scp = com_deb.scope(NS_DEBUG::ptr(this), __func__, "req",
//            NS_DEBUG::ptr(&req), "ctx", NS_DEBUG::ptr(&req->reserved()->operation_context_));
//        std::uint64_t         stag = make_tag64(tag, src);
//
//        auto& reg = ptr.handle_ref();
//#ifdef EXTRA_SIZE_CHECKS
//        if (size != reg.get_size())
//        {
//            OOMPH_DP_ONLY(com_err,
//                error(NS_DEBUG::str<>("recv mismatch"), "size", NS_DEBUG::hex<6>(size), "reg size",
//                    NS_DEBUG::hex<6>(reg.get_size())));
//        }
//#endif
//        m_context->get_controller()->recvs_posted_++;
//
//        // construct operation context in space reserved in request object
//        operation_context::cb_ptr_t cb_ptr = std::move(cb).release();
//        operation_context*          op_ctx = new (&req->reserved()->operation_context_)
//            operation_context(cb_ptr, &m_recv_cb_queue, &m_recv_cb_cancel);
//        assert(reinterpret_cast<void*>(op_ctx) ==
//               reinterpret_cast<void*>(&req->reserved()->operation_context_));
//
//        // clang-format off
//        OOMPH_DP_ONLY(com_deb,
//            debug(NS_DEBUG::str<>("Recv"),
//                  "thisrank", NS_DEBUG::dec<>(rank()),
//                  "rank", NS_DEBUG::dec<>(src),
//                  "tag", tag_disp(std::uint64_t(tag)),
//                  "ctag", tag_disp(m_ctag),
//                  "stag", tag_disp(stag),
//                  "addr", NS_DEBUG::ptr(reg.get_address()),
//                  "size", NS_DEBUG::hex<6>(size),
//                  "reg size", NS_DEBUG::hex<6>(reg.get_size()),
//                  "op_ctx", NS_DEBUG::ptr(op_ctx)));
//        // clang-format on
//
//        recv_tagged_region(reg, size, fi_addr_t(src), stag, op_ctx);
//        m_context->get_controller()->poll_recv_queue(m_rx_endpoint.get_rx_cq());
//    }

    shared_recv_request shared_recv(context_impl::heap_type::pointer& ptr, std::size_t size,
        rank_type src, tag_type tag, util::unique_function<void(rank_type, tag_type)>&& cb,
        std::atomic<std::size_t>* scheduled)
    {
        //[[maybe_unused]] auto scp = com_deb.scope(NS_DEBUG::ptr(this), __func__, "req",
        //    NS_DEBUG::ptr(&req), "ctx", NS_DEBUG::ptr(&req->reserved()->operation_context_));
        std::uint64_t         stag = make_tag64(tag, src);

        auto& reg = ptr.handle_ref();
#ifdef EXTRA_SIZE_CHECKS
        if (size != reg.get_size())
        {
            OOMPH_DP_ONLY(com_err,
                error(NS_DEBUG::str<>("recv mismatch"), "size", NS_DEBUG::hex<6>(size), "reg size",
                    NS_DEBUG::hex<6>(reg.get_size())));
        }
#endif
        m_context->get_controller()->recvs_posted_++;

        // construct request which is also an operation context
        auto s = std::make_shared<detail::shared_request_state>(m_context, this, scheduled, src,
            tag, std::move(cb));
        s->m_self_ptr = s;

        // clang-format off
        OOMPH_DP_ONLY(com_deb,
            debug(NS_DEBUG::str<>("Recv"),
                  "thisrank", NS_DEBUG::dec<>(rank()),
                  "rank", NS_DEBUG::dec<>(src),
                  "tag", tag_disp(std::uint64_t(tag)),
                  "ctag", tag_disp(m_ctag),
                  "stag", tag_disp(stag),
                  "addr", NS_DEBUG::ptr(reg.get_address()),
                  "size", NS_DEBUG::hex<6>(size),
                  "reg size", NS_DEBUG::hex<6>(reg.get_size()),
                  //"op_ctx", NS_DEBUG::ptr(op_ctx)));
                  "req", NS_DEBUG::ptr(s.get())));
        // clang-format on

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
                //[[maybe_unused]] auto scp =
                //    com_deb.scope("m_send_cb_queue.consume_all", q.user_cb_);
                auto ptr = std::move(req->m_self_ptr);
                req->invoke_cb();
            });

        m_recv_cb_queue.consume_all(
            [](oomph::detail::request_state* req)
            {
                //[[maybe_unused]] auto scp =
                //    com_deb.scope("m_recv_cb_queue.consume_all", q.user_cb_);
                auto ptr = std::move(req->m_self_ptr);
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
        OOMPH_DP_ONLY(com_deb,
            debug(NS_DEBUG::str<>("Cancel"), "ok", ok, "op_ctx", NS_DEBUG::ptr(op_ctx)));

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
                    OOMPH_DP_ONLY(com_deb, debug(NS_DEBUG::str<>("Cancel"), "succeeded", "op_ctx",
                                               NS_DEBUG::ptr(op_ctx)));
                    auto ptr = std::move(s->m_self_ptr);
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
    //bool cancel_recv_cb(recv_request const& req)
    //{
    //    // get the original message operation context
    //    operation_context* op_ctx =
    //        reinterpret_cast<operation_context*>(&req.m_data->reserved()->operation_context_);

    //    // replace the callback in the original message context with a cancel one
    //    //        mutable bool found = false;
    //    //        util::unique_function<void(void)> temp = [&](){ found = true; };
    //    //        auto orig_cb = std::exchange(op_ctx->user_cb_, temp.release());

    //    // submit the cancellation request
    //    bool ok = (fi_cancel(&m_rx_endpoint.get_ep()->fid, op_ctx) == 0);
    //    OOMPH_DP_ONLY(com_deb,
    //        debug(NS_DEBUG::str<>("Cancel"), "ok", ok, "op_ctx", NS_DEBUG::ptr(op_ctx)));

    //    // if the cancel operation failed completely, return
    //    if (!ok) return false;

    //    bool found = false;
    //    while (!found)
    //    {
    //        m_context->get_controller()->poll_recv_queue(m_rx_endpoint.get_rx_cq());
    //        // otherwise, poll until we know if it worked
    //        std::stack<libfabric::queue_data> temp_stack;
    //        libfabric::queue_data             temp;
    //        while (!found && m_recv_cb_cancel.pop(temp))
    //        {
    //            if (temp.ctxt == op_ctx)
    //            {
    //                // our recv was cancelled correctly
    //                found = true;
    //                delete op_ctx->user_cb_;
    //                OOMPH_DP_ONLY(com_deb, debug(NS_DEBUG::str<>("Cancel"), "succeeded", "op_ctx",
    //                                           NS_DEBUG::ptr(op_ctx)));
    //            }
    //            else
    //            {
    //                // a different cancel operation
    //                temp_stack.push(temp);
    //            }
    //        }
    //        // return any weird unhandled cancels back to the queue
    //        while (!temp_stack.empty())
    //        {
    //            libfabric::queue_data temp = temp_stack.top();
    //            temp_stack.pop();
    //            m_recv_cb_cancel.push(temp);
    //        }
    //    }
    //    return found;
    //}
    unsigned int max_tag() const noexcept { return 0x00000000007FFFFF; }
    unsigned int reserved_tag() const noexcept { return 0x0000000000800000; }
};

} // namespace oomph
