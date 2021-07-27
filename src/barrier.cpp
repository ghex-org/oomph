#include <oomph/barrier.hpp>


namespace oomph
{
barrier::barrier(size_t n_threads)
: m_threads{n_threads}
, b_count2{m_threads}
{
}

void
barrier::operator()(communicator& comm) const
{
    if (in_node1(comm)) rank_barrier(comm);
    else
        while (b_count2 == m_threads) comm.progress();
    in_node2(comm);
}

void
barrier::rank_barrier(communicator& comm) const
{
    MPI_Request req = MPI_REQUEST_NULL;
    int         flag;
    MPI_Ibarrier(comm.mpi_comm(), &req);
    while (true)
    {
        comm.progress();
        MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
        if (flag) break;
    }
}

bool
barrier::in_node1(communicator& comm) const
{
    size_t expected = b_count;
    while (!b_count.compare_exchange_weak(expected, expected + 1, std::memory_order_relaxed))
        expected = b_count;
    if (expected == m_threads - 1)
    {
        b_count.store(0);
        return true;
    }
    else
    {
        while (b_count != 0) { comm.progress(); }
        return false;
    }
}

void
barrier::in_node2(communicator& comm) const
{
    size_t ex = b_count2;
    while (!b_count2.compare_exchange_weak(ex, ex - 1, std::memory_order_relaxed)) ex = b_count2;
    if (ex == 1) { b_count2.store(m_threads); }
    else
    {
        while (b_count2 != m_threads) { comm.progress(); }
    }
}
} // namespace oomph
