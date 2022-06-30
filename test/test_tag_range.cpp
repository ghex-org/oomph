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
#include <gtest/gtest.h>
#include "./mpi_runner/mpi_test_fixture.hpp"
#include <oomph/context.hpp>

void
test_bits(oomph::util::tag_range r, oomph::tag_type t, oomph::tag_type bits)
{
    auto const wt = r.wrap(t);
    EXPECT_EQ(wt.get(), bits);
    EXPECT_EQ(wt.unwrap(), t);

    EXPECT_EQ(r.max_tag(), 0x080000);
}

TEST(tag_range, factory)
{
    using namespace oomph;

    //0x000005
    //0x280000
    util::tag_range_factory f(6, 23);

    test_bits(f.create(0), 0, 0x000000);
    test_bits(f.create(0), 1, 0x000001);
    test_bits(f.create(0), 10, 0x00000A);
    test_bits(f.create(0), 15, 0x00000F);
    test_bits(f.create(0, true), 0, 0x400000);
    test_bits(f.create(0, true), 1, 0x400001);
    test_bits(f.create(0, true), 10, 0x40000A);
    test_bits(f.create(0, true), 15, 0x40000F);

    test_bits(f.create(1), 0, 0x080000);
    test_bits(f.create(1), 1, 0x080001);
    test_bits(f.create(1), 10, 0x08000A);
    test_bits(f.create(1), 15, 0x08000F);
    test_bits(f.create(1, true), 0, 0x480000);
    test_bits(f.create(1, true), 1, 0x480001);
    test_bits(f.create(1, true), 10, 0x48000A);
    test_bits(f.create(1, true), 15, 0x48000F);

    test_bits(f.create(2), 0, 0x100000);
    test_bits(f.create(2), 1, 0x100001);
    test_bits(f.create(2), 10, 0x10000A);
    test_bits(f.create(2), 15, 0x10000F);
    test_bits(f.create(2, true), 0, 0x500000);
    test_bits(f.create(2, true), 1, 0x500001);
    test_bits(f.create(2, true), 10, 0x50000A);
    test_bits(f.create(2, true), 15, 0x50000F);

    test_bits(f.create(5), 0, 0x280000);
    test_bits(f.create(5), 1, 0x280001);
    test_bits(f.create(5), 10, 0x28000A);
    test_bits(f.create(5), 15, 0x28000F);
    test_bits(f.create(5, true), 0, 0x680000);
    test_bits(f.create(5, true), 1, 0x680001);
    test_bits(f.create(5, true), 10, 0x68000A);
    test_bits(f.create(5, true), 15, 0x68000F);
}



#define OOMPH_TEST_MSG_SIZE 512

template<typename Comms>
void test_send_recv(oomph::context& ctxt, Comms& comms, int src, int dst, int tag)
{
    using namespace oomph;

    std::vector<message_buffer<int>> s_buffers;
    std::vector<message_buffer<int>> r_buffers;
    std::vector<message_buffer<int>> s_buffers_reserved;
    std::vector<message_buffer<int>> r_buffers_reserved;

    for (std::size_t i = 0; i < comms.size(); ++i)
    {
        s_buffers.push_back(ctxt.make_buffer<int>(OOMPH_TEST_MSG_SIZE));
        for (auto& x : s_buffers.back()) x = comms[i].rank() * 100 + i;
        r_buffers.push_back(ctxt.make_buffer<int>(OOMPH_TEST_MSG_SIZE));
        for (auto& x : r_buffers.back()) x = -1;
        s_buffers_reserved.push_back(ctxt.make_buffer<int>(OOMPH_TEST_MSG_SIZE));
        for (auto& x : s_buffers_reserved.back()) x = comms[i].rank() * 10000 + i;
        r_buffers_reserved.push_back(ctxt.make_buffer<int>(OOMPH_TEST_MSG_SIZE));
        for (auto& x : r_buffers_reserved.back()) x = -1;
    }

    for (std::size_t i = 0; i < comms.size(); ++i)
        comms[i].recv(r_buffers_reserved[i], src, reserved(tag));
    for (std::size_t i = 0; i < comms.size(); ++i) comms[i].recv(r_buffers[i], src, tag);
    for (std::size_t i = comms.size(); i > 0; --i)
        comms[i - 1].send(s_buffers[i - 1], dst, tag).wait();
    for (std::size_t i = comms.size(); i > 0; --i)
        comms[i - 1].send(s_buffers_reserved[i - 1], dst, reserved(tag)).wait();

    for (auto& c : comms) c.wait_all();

    for (std::size_t i = 0; i < comms.size(); ++i)
        for (auto const& x : r_buffers[i]) EXPECT_EQ(x, src * 100 + i);

    for (std::size_t i = 0; i < comms.size(); ++i)
        for (auto const& x : r_buffers_reserved[i]) EXPECT_EQ(x, src * 10000 + i);
}

TEST(tag_range, send_recv)
{
    using namespace oomph;

    context ctxt(MPI_COMM_WORLD, false, 8);

    std::vector<communicator>        comms;
    comms.push_back(ctxt.get_communicator(0));
    comms.push_back(ctxt.get_communicator(1));
    comms.push_back(ctxt.get_communicator(2));
    comms.push_back(ctxt.get_communicator(7));

    rank_type dst = (comms[0].rank() + 1) % comms[0].size();
    rank_type src = (comms[0].rank() + (comms[0].size() - 1)) % comms[0].size();

    test_send_recv(ctxt, comms, src, dst, 0);
    test_send_recv(ctxt, comms, src, dst, 1);
    test_send_recv(ctxt, comms, src, dst, comms[0].tag_limit());
}
