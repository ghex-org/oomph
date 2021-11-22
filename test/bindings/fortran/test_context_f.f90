PROGRAM test_context
    !use iso_c_binding
    use oomph_mod
    use oomph_communicator_mod

    implicit none  

    include 'mpif.h'  

    integer :: mpi_err
    integer :: mpi_threading
    integer :: nthreads = 0
    type(oomph_communicator) :: comm
 
    call mpi_init_thread (MPI_THREAD_MULTIPLE, mpi_threading, mpi_err)

    ! init oomph
    call oomph_init(mpi_comm_world)

    comm = oomph_comm_new()

    call oomph_comm_free(comm)

    call oomph_finalize()

    call mpi_finalize(mpi_err)

END PROGRAM test_context
