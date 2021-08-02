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
#include <vector>
#include <thread>

void
test_1(oomph::communicator& comm, unsigned int size)
{
    EXPECT_TRUE(comm.size() == 4);
    auto msg = comm.make_buffer<int>(size);

    if (comm.rank() == 0)
    {
        for (unsigned int i = 0; i < size; ++i) msg[i] = i;
        std::vector<int> dsts = {1, 2, 3};
        auto             requests = comm.send_multi(msg, dsts, 42 + 42);
        for (auto& req : requests) req.wait();
    }
    else
    {
        auto req = comm.recv(msg, 0, 42);
        EXPECT_TRUE(req.cancel());
        comm.recv(msg, 0, 42 + 42).wait();
        for (unsigned int i = 0; i < size; ++i) EXPECT_EQ(msg[i], i);
    }
}

TEST_F(mpi_test_fixture, test_cancel_request)
{
    using namespace oomph;
    auto ctxt = context(MPI_COMM_WORLD);
    auto comm = ctxt.get_communicator();
    test_1(comm, 1);
    test_1(comm, 32);
    test_1(comm, 4096);
}

void
test_2(oomph::communicator& comm, unsigned int size)
{
    EXPECT_TRUE(comm.size() == 4);
    auto msg = comm.make_buffer<int>(size);
    using msg_t = decltype(msg);

    if (comm.rank() == 0)
    {
        for (unsigned int i = 0; i < size; ++i) msg[i] = i;
        std::vector<int> dsts = {1, 2, 3};
        auto             requests = comm.send_multi(msg, dsts, 42 + 42);
        for (auto& req : requests) req.wait();
    }
    else
    {
        int counter = 0;
        comm.recv(std::move(msg), 0, 42, [&counter](msg_t, int, int) { ++counter; });
        comm.progress();
        comm.progress();
        comm.progress();
        comm.progress();
        EXPECT_EQ(counter, 0);
        EXPECT_TRUE(comm.cancel_recv_cb(0, 42, [&counter, &msg](msg_t m, int, int) {
            ++counter;
            msg = std::move(m);
        }));
        EXPECT_EQ(counter, 1);
        EXPECT_EQ(msg.size(), size);
        comm.recv(msg, 0, 42 + 42).wait();
        for (unsigned int i = 0; i < size; ++i) EXPECT_EQ(msg[i], i);
    }
}

TEST_F(mpi_test_fixture, test_cancel_cb)
{
    using namespace oomph;
    auto ctxt = context(MPI_COMM_WORLD);
    auto comm = ctxt.get_communicator();
    test_2(comm, 1);
    test_2(comm, 32);
    test_2(comm, 4096);
}
