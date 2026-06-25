/*
 * ghex-org
 *
 * Copyright (c) 2014-2023, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <cstdint>
//
#include <thread>
// paths relative to backend
#include <communicator.hpp>
#include <context.hpp>
#include <controller.hpp>
#include <oomph_libfabric_defines.hpp>

namespace oomph
{
MAKE_LOGGER(src_log, "SRC")

using controller_type = libfabric::controller;

context_impl::context_impl(MPI_Comm comm, bool thread_safe,
    hwmalloc::heap_config const& heap_config, bool debug)
: context_base(comm, thread_safe)
, m_heap{this, heap_config}
, m_recv_cb_queue(128)
, m_recv_cb_cancel(8)
{
    libfatbat::log::init_from_env();
    //
    int rank, size;
    OOMPH_CHECK_MPI_RESULT(MPI_Comm_rank(comm, &rank));
    OOMPH_CHECK_MPI_RESULT(MPI_Comm_size(comm, &size));

    OOMPH_CHECK_MPI_RESULT(MPI_Bcast(&m_ctxt_tag, 1, MPI_UINT64_T, 0, comm));
    LIBFATBAT_DEBUG(src_log, "{:<20} rank {:03} context {}", "Broadcast", rank,
        static_cast<void*>(this));

    // TODO fix the thread safety
    // problem: controller is a singleton and has problems when 2 contexts are created
    // in the following order: single threaded first, then multi-threaded after
    // int threads = thread_safe ? std::thread::hardware_concurrency() : 1;
    // int threads = std::thread::hardware_concurrency();
    // Determine the number of threads based on the CPU affinity mask
    int threads = 1;
#if defined(_GNU_SOURCE)
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    if (sched_getaffinity(0, sizeof(cpuset), &cpuset) == 0) threads = CPU_COUNT(&cpuset);
    else // threads = boost::thread::physical_concurrency(); 
        threads = std::thread::hardware_concurrency();
#else
    threads = std::thread::hardware_concurrency();
#endif
    m_controller = init_libfabric_controller(this, comm, rank, size, threads, debug);
    m_domain = m_controller->get_domain();
}

communicator_impl*
context_impl::get_communicator()
{
    auto comm = new communicator_impl{this};
    m_comms_set.insert(comm);
    return comm;
}

char const*
context_impl::get_transport_option(std::string const& opt)
{
    if (opt == "name") { return "libfabric"; }
    else if (opt == "progress") { return libfabric_progress_string(); }
    else if (opt == "endpoint") { return libfabric_endpoint_string(); }
    else if (opt == "rendezvous_threshold")
    {
        static char buffer[32];
        std::string temp = std::to_string(m_controller->rendezvous_threshold());
        if (temp.size() > 31) throw std::runtime_error("Bad string option check, fix please");
        strncpy(buffer, temp.c_str(), 32);
        return buffer;
    }
    else
    {
        return "unspecified";
    }
}

std::shared_ptr<controller_type>
context_impl::init_libfabric_controller(oomph::context_impl* /*ctx*/, MPI_Comm comm, int rank,
    int size, int threads, bool debug)
{
    // only allow one thread to pass, make other wait
    static std::mutex                       m_init_mutex;
    std::lock_guard<std::mutex>             lock(m_init_mutex);
    static std::shared_ptr<controller_type> instance(nullptr);
    if (!instance.get())
    {
        LIBFATBAT_DEBUG(src_log, "{:<20} New Controller rank {:03} size {:03} threads {:03}",
            "New Controller", rank, size, threads);
        instance.reset(new controller_type());
        if (debug) instance->enable_debug();
        instance->initialize(HAVE_LIBFABRIC_PROVIDER, rank == 0, size, threads, comm);
    }
    return instance;
}

} // namespace oomph
