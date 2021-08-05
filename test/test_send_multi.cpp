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
#include <array>
#include <iomanip>
#include <thread>

const int SIZE = 1000000;

template<typename M>
void
init_msg(M& msg)
{
    for (std::size_t i = 0; i < msg.size(); ++i) msg[i] = i;
}

template<typename M>
bool
check_msg(M const& msg)
{
    bool ok = true;
    for (std::size_t i = 0; i < msg.size(); ++i) ok = ok && (msg[i] == (int)i);
    return ok;
}

TEST_F(mpi_test_fixture, send_multi)
{
    using namespace oomph;
    auto ctxt = context(MPI_COMM_WORLD);
    auto comm = ctxt.get_communicator();
    auto msg = comm.make_buffer<int>(SIZE);

    if (comm.size() < 2) return;

    if (comm.rank() == 0)
    {
        init_msg(msg);
        std::vector<int> dsts(comm.size() - 1);
        for (int i = 1; i < comm.size(); ++i) dsts[i - 1] = i;
        comm.send_multi(msg, dsts, 42).wait();
    }
    else
    {
        comm.recv(msg, 0, 42).wait();
        bool ok = check_msg(msg);
        EXPECT_TRUE(ok);
    }
}

TEST_F(mpi_test_fixture, send_multi_cb)
{
    using namespace oomph;
    auto ctxt = context(MPI_COMM_WORLD);
    auto comm = ctxt.get_communicator();
    auto msg = comm.make_buffer<int>(SIZE);

    if (comm.size() < 2) return;

    if (comm.rank() == 0)
    {
        init_msg(msg);
        std::vector<int> dsts(comm.size() - 1);
        for (int i = 1; i < comm.size(); ++i) dsts[i - 1] = i;
        bool arrived = false;
        comm.send_multi(std::move(msg), dsts, 42,
            [&arrived](message_buffer<int>, std::vector<int>, int) { arrived = true; });
        do
        {
            comm.progress();
        } while (!arrived);
    }
    else
    {
        comm.recv(msg, 0, 42).wait();
        bool ok = check_msg(msg);
        EXPECT_TRUE(ok);
    }
}
