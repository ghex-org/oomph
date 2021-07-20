#include <oomph/context.hpp>
#include <oomph/message_buffer.hpp>
//#include <oomph/channel.hpp>

#include <gtest/gtest.h>
#include "./mpi_test_fixture.hpp"

#include <thread>

TEST_F(mpi_test_fixture, construct)
{
    auto c = oomph::context(MPI_COMM_WORLD);

    //auto c2 = oomph::context(MPI_COMM_WORLD);

    //c2 = std::move(c);

    //auto b = oomph::make_buffer<int>(c, 10);

    // empty message buffer
    oomph::message_buffer<int> b;
    EXPECT_FALSE(b);

    // allocate buffer and move it to b
    b = c.make_buffer<int>(10);
    EXPECT_TRUE(b);

    for (auto& x : b) x = 99;

    for (auto x : b) std::cout << x << " ";
    std::cout << std::endl;

    auto comm = c.get_communicator();
    auto b2 = comm.make_buffer<int>(20);

    auto func = [&c]() { auto comm = c.get_communicator(); };

    std::vector<std::thread> threads;
    threads.push_back(std::thread{func});
    //threads.push_back( std::thread{func} );
    for (auto& t : threads) t.join();

    std::cout << "done" << std::endl;
}

TEST_F(mpi_test_fixture, sendrecv)
{
    auto c = oomph::context(MPI_COMM_WORLD);

    const auto send_rank = (world_rank + 1) % world_size;
    const auto send_tag = world_rank;
    const auto recv_rank = (world_rank + world_size - 1) % world_size;
    const auto recv_tag = recv_rank;

    auto recv_buffer = c.make_buffer<int>(10);
    for (auto& x : recv_buffer) x = -1;
    auto send_buffer = c.make_buffer<int>(10);
    for (auto& x : send_buffer) x = world_rank;

    auto comm = c.get_communicator();

    auto send_req = comm.send(send_buffer, send_rank, send_tag);
    auto recv_req = comm.recv(recv_buffer, recv_rank, recv_tag);

    send_req.wait();
    recv_req.wait();
}

//TEST_F(mpi_test_fixture, channel)
//{
//    auto c = oomph::context(MPI_COMM_WORLD);
//
//    const auto send_rank = (world_rank + 1) % world_size;
//    const auto send_tag = world_rank;
//    const auto recv_rank = (world_rank + world_size - 1) % world_size;
//    const auto recv_tag = recv_rank;
//
//    auto comm = c.get_communicator();
//
//    oomph::send_channel<int> s_channel(comm, 10, send_rank, send_tag, 1);
//    oomph::recv_channel<int> r_channel(comm, 10, recv_rank, recv_tag, 1);
//
//    s_channel.connect();
//    r_channel.connect();
//
//    typename oomph::recv_channel<int>::buffer rb;
//    rb = r_channel.get();
//}
