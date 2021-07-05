#include <oomph/context.hpp>
#include <oomph/message_buffer.hpp>

#include <gtest/gtest.h>
#include "./mpi_test_fixture.hpp"

#include <thread>

TEST_F(mpi_test_fixture, construct)
{
    auto c = oomph::context(MPI_COMM_WORLD);

    //auto c2 = oomph::context(MPI_COMM_WORLD);

    //c2 = std::move(c);

    //auto b = oomph::make_buffer<int>(c, 10);

    oomph::message_buffer<int> b;

    b = c.make_buffer<int>(10);

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
