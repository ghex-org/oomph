namespace oomph
{
context::context(MPI_Comm comm)
: m_mpi_comm{comm}
, m(m_mpi_comm.get())
{
}

context::context(context&&) = default;

context& context::operator=(context&&) = default;

context::~context() = default;

} // namespace oomph
