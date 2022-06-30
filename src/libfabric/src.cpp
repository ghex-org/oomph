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

context_impl::context_impl(MPI_Comm comm, bool thread_safe)
: context_base(comm, thread_safe)
, m_heap(this, false /*never_free*/)
{
    int rank, size;
    OOMPH_CHECK_MPI_RESULT(MPI_Comm_rank(comm, &rank));
    OOMPH_CHECK_MPI_RESULT(MPI_Comm_size(comm, &size));
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

} // namespace oomph

#include "../src.cpp"
