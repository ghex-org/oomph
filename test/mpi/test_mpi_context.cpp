#include <oomph/context.hpp>

#include <gtest/gtest.h>
#include "./mpi_test_fixture.hpp"

TEST_F(mpi_test_fixture, construct)
{
    auto c = oomph::context(MPI_COMM_WORLD);

    auto c2 = oomph::context(MPI_COMM_WORLD);

    c2 = std::move(c);
}
