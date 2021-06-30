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

#include "../mpi/mpi_test_fixture.hpp"
#include <ucp/api/ucp.h>
#include <iostream>

#ifdef NDEBUG
#define OOMPH_CHECK_UCX_RESULT_T(x) x;
#else
#include <string>
#include <stdexcept>
#define OOMPH_CHECK_UCX_RESULT_T(x)                                                               \
    if (x != UCS_OK)                                                                               \
        throw std::runtime_error("HWMALLOC Error: UCX Call failed " + std::string(#x) + " in " +   \
                                 std::string(__FILE__) + ":" + std::to_string(__LINE__));
#endif

struct ucx_test_fixture : public mpi_test_fixture
{
    static void SetUpTestSuite()
    {
        int num_ranks, rank;
        MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        if (rank == 0) std::cout << "initializing ucx..." << std::endl;

        // read run-time context
        ucp_config_t* config_ptr;
        OOMPH_CHECK_UCX_RESULT_T(ucp_config_read(NULL, NULL, &config_ptr));

        // set parameters
        ucp_params_t context_params;
        // define valid fields
        context_params.field_mask =
            UCP_PARAM_FIELD_FEATURES | // features
            //UCP_PARAM_FIELD_REQUEST_SIZE |      // size of reserved space in a non-blocking request
            UCP_PARAM_FIELD_TAG_SENDER_MASK |   // mask which gets sender endpoint from a tag
            UCP_PARAM_FIELD_MT_WORKERS_SHARED | // multi-threaded context: thread safety
            UCP_PARAM_FIELD_ESTIMATED_NUM_EPS;  // estimated number of endpoints for this context
        //UCP_PARAM_FIELD_REQUEST_INIT;       // initialize request memory

        // features
        context_params.features = UCP_FEATURE_TAG | // tag matching
                                  UCP_FEATURE_RMA;  // RMA access support
        // additional usable request size
        //context_params.request_size = request_data_size::value;
        // thread safety
        // this should be true if we have per-thread workers,
        // otherwise, if one worker is shared by all thread, it should be false
        // requires benchmarking.
        context_params.mt_workers_shared = true;
        // estimated number of connections
        // affects transport selection criteria and theresulting performance
        context_params.estimated_num_eps = num_ranks;
        // mask which specifies particular bits of the tag which can uniquely identify
        // the sender (UCP endpoint) in tagged operations.
        context_params.tag_sender_mask = 0xfffffffffffffffful;
        // needed to zero the memory region. Otherwise segfaults occured
        // when a std::function destructor was called on an invalid object
        //context_params.request_init = &request_init;

        //ucp_context_h ucp_context;
        //// initialize UCP
        OOMPH_CHECK_UCX_RESULT_T(ucp_init(&context_params, config_ptr, &ucp_context));
        if (rank == 0) ucp_config_print(config_ptr, stdout, NULL, UCS_CONFIG_PRINT_CONFIG);
        MPI_Barrier(MPI_COMM_WORLD);
        ucp_config_release(config_ptr);
    }

    static void TearDownTestSuite()
    {
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        if (rank == 0) std::cout << "cleaning up ucx..." << std::endl;
        MPI_Barrier(MPI_COMM_WORLD);
        ucp_cleanup(ucp_context);
    }

    //void SetUp() override {}
    //void TearDown(){}

  protected:
    static ucp_context_h ucp_context;
};

ucp_context_h ucx_test_fixture::ucp_context;
