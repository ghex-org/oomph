#include "./rank_topology.hpp"

namespace oomph
{
rank_topology::rank_topology(MPI_Comm comm)
: m_comm(comm)
{
    // get rank from comm
    int rank;
    OOMPH_CHECK_MPI_RESULT(MPI_Comm_rank(comm, &rank));
    // split comm into shared memory comms
    const int key = rank;
    OOMPH_CHECK_MPI_RESULT(
        MPI_Comm_split_type(comm, MPI_COMM_TYPE_SHARED, key, MPI_INFO_NULL, &m_shared_comm));
    // get rank within shared memory comm and its size
    OOMPH_CHECK_MPI_RESULT(MPI_Comm_rank(m_shared_comm, &m_rank));
    int size;
    OOMPH_CHECK_MPI_RESULT(MPI_Comm_size(m_shared_comm, &size));
    // gather rank info from all ranks within shared comm
    std::vector<int> ranks(size);
    MPI_Allgather(&rank, 1, MPI_INT, ranks.data(), 1, MPI_INT, m_shared_comm);
    // insert into set
    for (auto r : ranks) m_rank_set.insert(r);
    OOMPH_CHECK_MPI_RESULT(MPI_Comm_free(&m_shared_comm));
}
} //namespace oomph
