PROGRAM test_context
  use mpi
  use oomph_mod
  use oomph_communicator_mod

  implicit none  

  integer :: mpi_err
  integer :: mpi_threading
  type(oomph_communicator) :: comm

  call mpi_init_thread (MPI_THREAD_SINGLE, mpi_threading, mpi_err)

  ! init oomph
  call oomph_init(1, mpi_comm_world)

  comm = oomph_get_communicator()
  print *, "rank: ", oomph_comm_rank(comm)

  ! cleanup
  print *, "result: OK"
  call oomph_free(comm)
  call oomph_finalize()
  call mpi_finalize(mpi_err)

END PROGRAM test_context
