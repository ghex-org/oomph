#include <oomph/context.hpp>
#include <gtest/gtest.h>
#include "./mpi_runner/mpi_test_fixture.hpp"

TEST_F(mpi_test_fixture, comm_log_two_groups)
{
    using namespace oomph;
    auto ctxt = context(MPI_COMM_WORLD, false);
    auto comm = ctxt.get_communicator();

    int rank = comm.rank();
    int size = comm.size();
    int next_rank = (rank + 1) % size;
    int prev_rank = (rank + size - 1) % size;

    constexpr std::size_t buf_size = 1000;

    auto buf_send1 = comm.make_buffer<int>(buf_size);
    auto buf_recv1 = comm.make_buffer<int>(buf_size);
    for (auto& x : buf_send1) x = rank;

    comm.start_group();
    comm.send(buf_send1, next_rank, 0);
    comm.recv(buf_recv1, prev_rank, 0);
    comm.end_group();
    comm.wait_all();

    auto buf_send2 = comm.make_buffer<int>(buf_size * 2);
    auto buf_recv2 = comm.make_buffer<int>(buf_size * 2);
    for (auto& x : buf_send2) x = rank + 100;

    comm.start_group();
    comm.send(buf_send2, next_rank, 1);
    comm.recv(buf_recv2, prev_rank, 1);
    comm.end_group();
    comm.wait_all();
}
