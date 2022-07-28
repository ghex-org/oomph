/*
 * ghex-org
 *
 * Copyright (c) 2014-2022, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "oomph_libfabric_defines.hpp"
#include "controller.hpp"
#include "communicator.hpp"
#include "context.hpp"

namespace oomph
{
// cppcheck-suppress ConfigurationNotChecked
static NS_DEBUG::enable_print<false> src_deb("__SRC__");

using controller_type = oomph::libfabric::controller;

context_impl::context_impl(MPI_Comm comm, bool thread_safe, bool message_pool_never_free,
    std::size_t message_pool_reserve)
: context_base(comm, thread_safe)
, m_heap{this, message_pool_never_free, message_pool_reserve}
, m_recv_cb_queue(128)
, m_recv_cb_cancel(8)
{
    int rank, size;
    OOMPH_CHECK_MPI_RESULT(MPI_Comm_rank(comm, &rank));
    OOMPH_CHECK_MPI_RESULT(MPI_Comm_size(comm, &size));
    // TODO fix the thread safety
    // problem: controller is a singleton and has problems when 2 contexts are created in the
    // following order: single threaded first, then multi-threaded after
    //int threads = thread_safe ? std::thread::hardware_concurrency() : 1;
    int threads = std::thread::hardware_concurrency();
    m_controller = init_libfabric_controller(this, comm, rank, size, threads);
    m_domain = m_controller->get_domain();
}

communicator_impl*
context_impl::get_communicator()
{
    auto comm = new communicator_impl{this};
    m_comms_set.insert(comm);
    return comm;
}

const char*
context_impl::get_transport_option(const std::string& opt)
{
    if (opt == "name") { return "libfabric"; }
    else if (opt == "progress") { return libfabric_progress_string(); }
    else if (opt == "endpoint") { return libfabric_endpoint_string(); }
    else if (opt == "rendezvous_threshold")
    {
        static char buffer[32];
        std::string temp = std::to_string(m_controller->rendezvous_threshold());
        strncpy(buffer, temp.c_str(), std::min(size_t(31), std::strlen(temp.c_str())));
        return buffer;
    }
    else { return "unspecified"; }
}

std::shared_ptr<controller_type>
context_impl::init_libfabric_controller(oomph::context_impl* /*ctx*/, MPI_Comm comm, int rank,
    int size, int threads)
{
    // only allow one thread to pass, make other wait
    static std::mutex                       m_init_mutex;
    std::lock_guard<std::mutex>             lock(m_init_mutex);
    static std::shared_ptr<controller_type> instance(nullptr);
    if (!instance.get())
    {
        OOMPH_DP_ONLY(src_deb, debug(NS_DEBUG::str<>("New Controller"), "rank", debug::dec<3>(rank),
                                   "size", debug::dec<3>(size), "threads", debug::dec<3>(threads)));
        instance.reset(new controller_type());
        instance->initialize(HAVE_LIBFABRIC_PROVIDER, rank == 0, size, threads, comm);
    }
    return instance;
}

namespace libfabric
{
void
operation_context::handle_cancelled()
{
    [[maybe_unused]] auto scp = ctx_deb.scope(NS_DEBUG::ptr(this), __func__);
    // enqueue the cancelled/callback
    if (m_req.index() == 0)
    {
        // regular (non-shared) recv
        auto s = std::get<0>(m_req);
        while (!(s->m_comm->m_recv_cb_cancel.push(s))) {}
    }
    else
    {
        // shared recv
        auto s = std::get<1>(m_req);

        while (!(s->m_ctxt->m_recv_cb_cancel.push(s))) {}
    }
}

int
operation_context::handle_tagged_recv_completion_impl(void* user_data)
{
    [[maybe_unused]] auto scp = ctx_deb.scope(NS_DEBUG::ptr(this), __func__);
    if (m_req.index() == 0)
    {
        // regular (non-shared) recv
        auto s = std::get<0>(m_req);
        //if (std::this_thread::get_id() == thread_id_)
        if (reinterpret_cast<oomph::communicator_impl*>(user_data) == s->m_comm)
        {
            if (!s->m_comm->has_reached_recursion_depth())
            {
                auto inc = s->m_comm->recursion();
                auto ptr = s->release_self_ref();
                s->invoke_cb();
            }
            else
            {
                // enqueue the callback
                while (!(s->m_comm->m_recv_cb_queue.push(s))) {}
            }
        }
        else
        {
            // enqueue the callback
            while (!(s->m_comm->m_recv_cb_queue.push(s))) {}
        }
    }
    else
    {
        // shared recv
        auto s = std::get<1>(m_req);
        if (!s->m_comm->m_context->has_reached_recursion_depth())
        {
            auto inc = s->m_comm->m_context->recursion();
            auto ptr = s->release_self_ref();
            s->invoke_cb();
        }
        else
        {
            // enqueue the callback
            while (!(s->m_comm->m_context->m_recv_cb_queue.push(s))) {}
        }
    }
    return 1;
}

int
operation_context::handle_tagged_send_completion_impl(void* user_data)
{
    auto s = std::get<0>(m_req);
    if (reinterpret_cast<oomph::communicator_impl*>(user_data) == s->m_comm)
    {
        if (!s->m_comm->has_reached_recursion_depth())
        {
            auto inc = s->m_comm->recursion();
            auto ptr = s->release_self_ref();
            s->invoke_cb();
        }
        else
        {
            // enqueue the callback
            while (!(s->m_comm->m_send_cb_queue.push(s))) {}
        }
    }
    else
    {
        // enqueue the callback
        while (!(s->m_comm->m_send_cb_queue.push(s))) {}
    }
    return 1;
}
} // namespace libfabric
} // namespace oomph

#include "../src.cpp"
