/*
 * GridTools
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <oomph/context.hpp>
#include <gtest/gtest.h>
#include "./mpi_runner/mpi_test_fixture.hpp"
#include <iostream>
#include <iomanip>
#include <thread>

#define NITERS 100

TEST_F(mpi_test_fixture, ring_send_recv)
{
    using namespace oomph;
    auto ctxt = context(MPI_COMM_WORLD);
    auto comm = ctxt.get_communicator();
    auto smsg = comm.make_buffer<int>(1);
    auto rmsg = comm.make_buffer<int>(1);
    int  speer_rank = (comm.rank() + 1) % comm.size();
    int  rpeer_rank = (comm.rank() + comm.size() - 1) % comm.size();

    smsg[0] = comm.rank();
    rmsg[0] = -1;

    for (int i = 0; i < NITERS; i++)
    {
        auto rreq = comm.recv(rmsg, rpeer_rank, 1);
        auto sreq = comm.send(smsg, speer_rank, 1);
        while (!(rreq.is_ready() && sreq.is_ready())) {};
        EXPECT_EQ(rmsg[0], rpeer_rank);
        rmsg[0] = -1;
    }
}

TEST_F(mpi_test_fixture, ring_send_recv_cb)
{
    using namespace oomph;
    auto ctxt = context(MPI_COMM_WORLD);
    auto comm = ctxt.get_communicator();
    auto smsg = comm.make_buffer<int>(1);
    auto rmsg = comm.make_buffer<int>(1);
    int  speer_rank = (comm.rank() + 1) % comm.size();
    int  rpeer_rank = (comm.rank() + comm.size() - 1) % comm.size();

    smsg[0] = comm.rank();
    rmsg[0] = -1;

    volatile int received = 0;
    volatile int sent = 0;

    auto send_callback = [&](message_buffer<int> msg, int, int) {
        ++sent;
        smsg = std::move(msg);
    };

    auto recv_callback = [&](message_buffer<int> msg, int, int) {
        ++received;
        rmsg = std::move(msg);
    };

    for (int i = 0; i < NITERS; i++)
    {
        comm.recv(std::move(rmsg), rpeer_rank, 1, recv_callback);
        comm.send(std::move(smsg), speer_rank, 1, send_callback);
        while (received <= i || sent <= i) { comm.progress(); }
        EXPECT_EQ(rmsg[0], rpeer_rank);
        rmsg[0] = -1;
    }
    EXPECT_EQ(received, NITERS);
    EXPECT_EQ(sent, NITERS);
}

TEST_F(mpi_test_fixture, ring_send_recv_cb_resubmit)
{
    using namespace oomph;
    auto ctxt = context(MPI_COMM_WORLD);
    auto comm = ctxt.get_communicator();
    auto smsg = comm.make_buffer<int>(1);
    auto rmsg = comm.make_buffer<int>(1);
    int  speer_rank = (comm.rank() + 1) % comm.size();
    int  rpeer_rank = (comm.rank() + comm.size() - 1) % comm.size();

    smsg[0] = comm.rank();
    rmsg[0] = -1;

    volatile int received = 0;

    struct recursive_recv_callback
    {
        communicator& comm;
        volatile int& received;

        void operator()(oomph::message_buffer<int> msg, int src, int tag)
        {
            ++received;
            EXPECT_EQ(msg[0], src);
            msg[0] = -1;
            if (received < NITERS)
                comm.recv(std::move(msg), src, tag, recursive_recv_callback{*this});
        }
    };

    comm.recv(std::move(rmsg), rpeer_rank, 1, recursive_recv_callback{comm, received});

    for (int i = 0; i < NITERS; i++)
    {
        auto sreq = comm.send(smsg, speer_rank, 1);
        while (!sreq.is_ready() || received <= i) { comm.progress(); };
    }
    EXPECT_EQ(received, NITERS);
}
