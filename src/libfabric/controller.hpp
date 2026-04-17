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

#include <mutex>
#include <string>
#include <vector>
//
#include <cstddef>
#include <cstdint>
#include <cstring>
//
#include <rdma/fabric.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_eq.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_rma.h>
#include <rdma/fi_tagged.h>
//
#include <libfatbat/controller_base.hpp>
#include <libfatbat/fabric_error.hpp>
#include <libfatbat/locality.hpp>
#include <libfatbat/logging.hpp>
//
#include "oomph_libfabric_defines.hpp"
#include "operation_context.hpp"
//
#include <oomph/util/unique_function.hpp>
//
#include <mpi.h>

namespace oomph::libfabric
{
inline auto ctrl_log = libfatbat::log::create("Ctrl");

class controller : public libfatbat::controller_base<controller>
{
  public:
    // --------------------------------------------------------------------
    controller()
    : libfatbat::controller_base<controller>()
    {
    }

    // --------------------------------------------------------------------
    void initialize_derived(std::string const&, bool, int, size_t, MPI_Comm mpi_comm)
    {
        // Broadcast address of all endpoints to all ranks
        // and fill address vector with info
        exchange_addresses(av_, mpi_comm);
    }

    // --------------------------------------------------------------------
    constexpr fi_threading threadlevel_flags()
    {
#if defined(HAVE_LIBFABRIC_GNI) || defined(HAVE_LIBFABRIC_LNX)
        return FI_THREAD_ENDPOINT;
#else
        return FI_THREAD_SAFE;
#endif
    }

    // --------------------------------------------------------------------
    uint64_t caps_flags(uint64_t /*available_flags*/) const
    {
        uint64_t flags_required = FI_TAGGED;
#ifndef HAVE_LIBFABRIC_LNX
        flags_required |= FI_MSG | FI_TAGGED | FI_RECV | FI_SEND | FI_RMA | FI_READ | FI_WRITE |
                          FI_REMOTE_READ | FI_REMOTE_WRITE;
#if OOMPH_ENABLE_DEVICE
        flags_required |= FI_HMEM;
#endif
#endif
        return flags_required;
    }

    // --------------------------------------------------------------------
    // we do not need to perform any special actions on init (to contact root node)
    void setup_root_node_address(struct fi_info* /*info*/) {}

    // --------------------------------------------------------------------
    // send address to rank 0 and receive array of all localities
    void MPI_exchange_localities(fid_av* av, MPI_Comm comm, int rank, int size)
    {
        LIBFATBAT_SCOPE(ctrl_log, "{} {}", static_cast<void*>(this), __func__);

        // array of empty locality objects
        std::vector<libfatbat::locality::locality_data> localities(size);
        //
        if (rank > 0)
        {
            LIBFATBAT_DEBUG(ctrl_log, "{:<20} {} {} size {}", "sending here", here_.to_str(),
                static_cast<const void*>(here_.fabric_data().data()),
                libfatbat::locality_defs::array_size);
            /*int err = */ MPI_Send(here_.fabric_data().data(),
                libfatbat::locality_defs::array_size, MPI_CHAR,
                0, // dst rank
                0, // tag
                comm);

            LIBFATBAT_DEBUG(ctrl_log, "{:<20} {} {} size {}", "receiving all", "", "",
                libfatbat::locality_defs::array_size);

            MPI_Status status;
            /*err = */ MPI_Recv(localities.data(), size * libfatbat::locality_defs::array_size,
                MPI_CHAR,
                0, // src rank
                0, // tag
                comm, &status);
            LIBFATBAT_DEBUG(ctrl_log, "{:<20} {} {} size {}", "received addresses", "", "",
                libfatbat::locality_defs::array_size);
        }
        else
        {
            LIBFATBAT_DEBUG(ctrl_log, "{:<20} {} {} size {}", "receiving addresses", "", "",
                libfatbat::locality_defs::array_size);
            memcpy(&localities[0], here_.fabric_data().data(),
                libfatbat::locality_defs::array_size);
            for (int i = 1; i < size; ++i)
            {
                LIBFATBAT_DEBUG(ctrl_log, "{:<20} {} {} size {}", "receiving address", i, "",
                    libfatbat::locality_defs::array_size);
                MPI_Status status;
                /*int err = */ MPI_Recv(&localities[i], size * libfatbat::locality_defs::array_size,
                    MPI_CHAR,
                    i, // src rank
                    0, // tag
                    comm, &status);
                LIBFATBAT_DEBUG(ctrl_log, "{:<20} {} {} size {}", "received address", i, "",
                    libfatbat::locality_defs::array_size);
            }

            LIBFATBAT_DEBUG(ctrl_log, "{:<20} {} {} size {}", "sending all", "", "",
                libfatbat::locality_defs::array_size);
            for (int i = 1; i < size; ++i)
            {
                LIBFATBAT_DEBUG(ctrl_log, "{:<20} {} {} size {}", "sending to", i, "",
                    libfatbat::locality_defs::array_size);
                /*int err = */ MPI_Send(&localities[0], size * libfatbat::locality_defs::array_size,
                    MPI_CHAR,
                    i, // dst rank
                    0, // tag
                    comm);
            }
        }

        // all ranks should now have a full localities vector
        LIBFATBAT_DEBUG(ctrl_log, "{:<20} size {}", "populating vector",
            libfatbat::locality_defs::array_size);
        for (int i = 0; i < size; ++i)
        {
            libfatbat::locality temp(localities[i], av);
            insert_address(temp);
        }
    }

    // --------------------------------------------------------------------
    // if we did not bootstrap, then fetch the list of all localities
    // and insert each one into the address vector
    void exchange_addresses(fid_av* av, MPI_Comm mpi_comm)
    {
        LIBFATBAT_SCOPE(ctrl_log, "{} {}", static_cast<void*>(this), __func__);

        int rank, size;
        MPI_Comm_rank(mpi_comm, &rank);
        MPI_Comm_size(mpi_comm, &size);

        LIBFATBAT_DEBUG(ctrl_log, "{:<20} size {}", "initialize_localities", size);

        MPI_exchange_localities(av, mpi_comm, rank, size);
#ifndef HAVE_LIBFABRIC_LNX // address stuff not yet supported
        debug_print_av_vector(size);
#endif
        LIBFATBAT_DEBUG(ctrl_log, "{:<20} size {}", "Done localities", size);
    }

    // --------------------------------------------------------------------
    inline constexpr bool bypass_tx_lock()
    {
#if defined(HAVE_LIBFABRIC_GNI)
        return true;
#elif defined(HAVE_LIBFABRIC_LNX)
        // @todo : cxi provider is not yet thread safe using scalable endpoints
        return false;
#else
        return (threadlevel_flags() == FI_THREAD_SAFE ||
                endpoint_type_ == endpoint_type::threadlocalTx);
#endif
    }

    // --------------------------------------------------------------------
    inline controller_base::unique_lock get_tx_lock()
    {
        if (bypass_tx_lock()) return unique_lock();
        return unique_lock(send_mutex_);
    }

    // --------------------------------------------------------------------
    inline controller_base::unique_lock try_tx_lock()
    {
        if (bypass_tx_lock()) return unique_lock();
        return unique_lock(send_mutex_, std::try_to_lock_t{});
    }

    // --------------------------------------------------------------------
    inline constexpr bool bypass_rx_lock()
    {
#ifdef HAVE_LIBFABRIC_GNI
        return true;
#else
        return (
            threadlevel_flags() == FI_THREAD_SAFE || endpoint_type_ == endpoint_type::scalableTxRx);
#endif
    }

    // --------------------------------------------------------------------
    inline controller_base::unique_lock get_rx_lock()
    {
        if (bypass_rx_lock()) return unique_lock();
        return unique_lock(recv_mutex_);
    }

    // --------------------------------------------------------------------
    inline controller_base::unique_lock try_rx_lock()
    {
        if (bypass_rx_lock()) return unique_lock();
        return unique_lock(recv_mutex_, std::try_to_lock_t{});
    }

    // --------------------------------------------------------------------
    int poll_send_queue(fid_cq* send_cq, void* user_data)
    {
#ifdef EXCESSIVE_POLLING_BACKOFF_MICRO_S
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::microseconds>(now - send_poll_stamp).count() <
            EXCESSIVE_POLLING_BACKOFF_MICRO_S)
            return 0;
        send_poll_stamp = now;
#endif
        int             ret;
        fi_cq_msg_entry entry[max_completions_array_limit_];
        assert(max_completions_per_poll_ <= max_completions_array_limit_);
        {
            auto lock = try_tx_lock();

            // if we're not threadlocal and didn't get the lock,
            // then another thread is polling now, just exit
            if (!bypass_tx_lock() && !lock.owns_lock()) { return -1; }

            // static auto polling =
            //     NS_DEBUG::cnt_deb<9>.make_timer(1, NS_DEBUG::str<>("poll send queue"));
            // LF_DEB(cnt_deb<9>, timed(polling, static_cast<void*>(send_cq)));

            // poll for completions
            {
                ret = fi_cq_read(send_cq, &entry[0], max_completions_per_poll_);
            }
            // if there is an error, retrieve it
            if (ret == -FI_EAVAIL)
            {
                struct fi_cq_err_entry e = {};
                int                    err_sz = fi_cq_readerr(send_cq, &e, 0);
                (void)err_sz;

                // flags might not be set correctly
                if ((e.flags & (FI_MSG | FI_SEND | FI_TAGGED)) != 0)
                {
                    LIBFATBAT_ERROR(ctrl_log,
                        "{:<20} Error FI_EAVAIL for FI_SEND with len {:#06x} context {} errcode {:3} flags {:16b} error {}",
                        "txcq", e.len, static_cast<void*>(e.op_context), e.err, e.flags,
                        fi_cq_strerror(send_cq, e.prov_errno, e.err_data, (char*)e.buf, e.len));
                }
                else if ((e.flags & FI_RMA) != 0)
                {
                    LIBFATBAT_ERROR(ctrl_log,
                        "{:<20} Error FI_EAVAIL for FI_RMA with len {:#06x} context {} errcode {:3} flags {:16b} error {}",
                        "txcq", e.len, static_cast<void*>(e.op_context), e.err, e.flags,
                        fi_cq_strerror(send_cq, e.prov_errno, e.err_data, (char*)e.buf, e.len));
                }
                operation_context* handler = reinterpret_cast<operation_context*>(e.op_context);
                handler->handle_error(e);
                return 0;
            }
        }
        //
        // exit possibly locked region and process each completion
        //
        if (ret > 0)
        {
            [[maybe_unused]] std::array<char, 1024> buf;
            int                                     processed = 0;
            for (int i = 0; i < ret; ++i)
            {
                ++sends_complete_;
                LIBFATBAT_DEBUG(ctrl_log, "{:<20} {} {} length {:#06x}", "Completion", i,
                    static_cast<void*>(entry[i].op_context), entry[i].len);
                if ((entry[i].flags & (FI_TAGGED | FI_SEND | FI_MSG)) != 0)
                {
                    LIBFATBAT_DEBUG(ctrl_log, "{:<20} {} {} {}", "Completion",
                        "txcq tagged send completion", static_cast<void*>(entry[i].op_context), "");

                    operation_context* handler =
                        reinterpret_cast<operation_context*>(entry[i].op_context);
                    processed += handler->handle_tagged_send_completion(user_data);
                }
                else
                {
                    LIBFATBAT_DEBUG(ctrl_log, "{:<20} {} {} {}", "Completion",
                        "unknown txcq completion", static_cast<void*>(entry[i].op_context), "");
                    std::terminate();
                }
            }
            return processed;
        }
        else if (ret == 0 || ret == -FI_EAGAIN)
        {
            // do nothing, we will try again on the next check
        }
        else
        {
            LIBFATBAT_DEBUG(ctrl_log, "{:<20} {} {} {}", "Completion",
                "unknown error in completion txcq read", static_cast<void*>(entry[0].op_context),
                "");
        }
        return 0;
    }

    // --------------------------------------------------------------------
    int poll_recv_queue(fid_cq* rx_cq, void* user_data)
    {
#ifdef EXCESSIVE_POLLING_BACKOFF_MICRO_S
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::microseconds>(now - recv_poll_stamp).count() <
            EXCESSIVE_POLLING_BACKOFF_MICRO_S)
            return 0;
        recv_poll_stamp = now;
#endif
        int             ret;
        fi_cq_msg_entry entry[max_completions_array_limit_];
        assert(max_completions_per_poll_ <= max_completions_array_limit_);
        {
            auto lock = get_rx_lock();

            // if we're not threadlocal and didn't get the lock,
            // then another thread is polling now, just exit
            if (!bypass_rx_lock() && !lock.owns_lock()) { return -1; }

            // static auto polling =
            //     NS_DEBUG::cnt_deb<2>.make_timer(1, NS_DEBUG::str<>("poll recv queue"));
            // LF_DEB(cnt_deb<2>, timed(polling, static_cast<void*>(rx_cq)));

            // poll for completions
            {
                ret = fi_cq_read(rx_cq, &entry[0], max_completions_per_poll_);
            }
            // if there is an error, retrieve it
            if (ret == -FI_EAVAIL)
            {
                // read the full error status
                struct fi_cq_err_entry e = {};
                int                    err_sz = fi_cq_readerr(rx_cq, &e, 0);
                (void)err_sz;
                // from the manpage 'man 3 fi_cq_readerr'
                if (e.err == FI_ECANCELED)
                {
                    LIBFATBAT_DEBUG(ctrl_log, "{:<20} flags {:#06x} len {:#06x} context {}",
                        "rxcq Cancelled", e.flags, e.len, static_cast<void*>(e.op_context));
                    // the request was cancelled, we can simply exit
                    // as the canceller will have doone any cleanup needed
                    operation_context* handler = reinterpret_cast<operation_context*>(e.op_context);
                    handler->handle_cancelled();
                    return 0;
                }
                else if (e.err != FI_SUCCESS)
                {
                    LIBFATBAT_DEBUG(ctrl_log,
                        "{:<20} error code {} flags {:#06x} len {:#06x} context {} error msg {}",
                        "poll_recv_queue", -e.err, e.flags, e.len, static_cast<void*>(e.op_context),
                        fi_cq_strerror(rx_cq, e.prov_errno, e.err_data, (char*)e.buf, e.len));
                }
                operation_context* handler = reinterpret_cast<operation_context*>(e.op_context);
                if (handler) handler->handle_error(e);
                return 0;
            }
        }
        //
        // release the lock and process each completion
        //
        if (ret > 0)
        {
            std::array<char, 1024> buf;
            int                    processed = 0;
            for (int i = 0; i < ret; ++i)
            {
                ++recvs_complete_;
                LIBFATBAT_DEBUG(ctrl_log,
                    "{:<20} {:02} {} flags {} ({:#06x}) context {} length {:#06x}",
                    "Completion txcq", i,
                    fi_tostr_r(buf.data(), buf.size(), &entry[i].flags, FI_TYPE_CQ_EVENT_FLAGS),
                    entry[i].flags, static_cast<void*>(entry[i].op_context), entry[i].len);

                if ((entry[i].flags & (FI_TAGGED | FI_RECV)) != 0)
                {
                    LIBFATBAT_DEBUG(ctrl_log, "{:<20} {} {} {}", "Completion", i,
                        static_cast<void*>(entry[i].op_context), "rxcq tagged recv completion");

                    operation_context* handler =
                        reinterpret_cast<operation_context*>(entry[i].op_context);
                    processed += handler->handle_tagged_recv_completion(user_data);
                }
                else
                {
                    LIBFATBAT_DEBUG(ctrl_log, "{:<20} {} {} {}", "Completion", i,
                        static_cast<void*>(entry[i].op_context),
                        "Received an unknown rxcq completion");
                    std::terminate();
                }
            }
            return processed;
        }
        else if (ret == 0 || ret == -FI_EAGAIN)
        {
            // do nothing, we will try again on the next check
        }
        else
        {
            LIBFATBAT_ERROR(ctrl_log, "{:<20} unknown error in completion rxcq read", "Completion");
        }
        return 0;
    }

    // Jobs started using mpi don't have this info
    struct fi_info* set_src_dst_addresses(struct fi_info* info, bool tx)
    {
        (void)info; // unused variable warning
        (void)tx;   // unused variable warning

        LIBFATBAT_DEBUG(ctrl_log, "{:<20} {} {} {}", "fi_dupinfo", 0, 0, "called");
        struct fi_info* hints = fi_dupinfo(info);
        if (!hints) throw libfatbat::fabric_error(0, "fi_dupinfo");
        // clear any Rx address data that might be set
        // free(hints->src_addr);
        // hints->src_addr = nullptr;
        // hints->src_addrlen = 0;
        free(hints->dest_addr);
        hints->dest_addr = nullptr;
        hints->dest_addrlen = 0;
        return hints;
    }
};

} // namespace oomph::libfabric
