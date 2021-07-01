#include <oomph/context.hpp>
#include <oomph/message_buffer.hpp>

#include <gtest/gtest.h>
#include "./mpi_test_fixture.hpp"

TEST_F(mpi_test_fixture, construct)
{
    auto c = oomph::context(MPI_COMM_WORLD);

    //auto c2 = oomph::context(MPI_COMM_WORLD);

    //c2 = std::move(c);

    //auto b = oomph::make_buffer<int>(c, 10);
    auto b = c.make_buffer<int>(10);

    for (auto& x : b) x = 99;

    for (auto x : b) std::cout << x << " ";
    std::cout << std::endl;

    std::cout << "done" << std::endl;
}
