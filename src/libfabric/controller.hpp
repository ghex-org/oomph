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

class controller : public libfatbat::controller_base<controller, operation_context>
{
  public:
    // --------------------------------------------------------------------
    controller()
    : libfatbat::controller_base<controller, operation_context>()
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

        LIBFATBAT_DEBUG(ctrl_log, "{:<20} size {}", "init_localities", size);

        MPI_exchange_localities(av, mpi_comm, rank, size);
#ifndef HAVE_LIBFABRIC_LNX // address stuff not yet supported
        debug_print_av_vector(size);
#endif
        LIBFATBAT_DEBUG(ctrl_log, "{:<20} size {}", "Done localities", size);
    }

    // --------------------------------------------------------------------
    int poll_send_queue(fid_cq* send_cq, void* user_data)
    {
        return static_cast<controller_base*>(this)->poll_send_queue_default(send_cq, user_data);
    }

    // --------------------------------------------------------------------
    int poll_recv_queue(fid_cq* rx_cq, void* user_data)
    {
        return static_cast<controller_base*>(this)->poll_recv_queue_default(rx_cq, user_data);
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
