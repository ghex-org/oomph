PROGRAM test_send_recv_ft
  use mpi
  use omp_lib
  use oomph_mod

  implicit none  

  integer :: mpi_err
  integer :: mpi_threading, mpi_size, mpi_rank, mpi_peer
  integer :: nthreads = 0, thrid

  type(oomph_communicator) :: comm
  type(oomph_request) :: sreq, rreq

  ! message
  integer(8) :: msg_size = 16
  type(oomph_message) :: smsg, rmsg
  integer(1), dimension(:), pointer :: msg_data

  !$omp parallel shared(nthreads)
  nthreads = omp_get_num_threads()
  !$omp end parallel

  if (nthreads==1) then
    call mpi_init_thread (MPI_THREAD_SINGLE, mpi_threading, mpi_err)
  else
    call mpi_init_thread (MPI_THREAD_MULTIPLE, mpi_threading, mpi_err)
    if (MPI_THREAD_MULTIPLE /= mpi_threading) then
      stop "MPI does not support multithreading"
    end if
  end if
  call mpi_comm_size (mpi_comm_world, mpi_size, mpi_err)
  call mpi_comm_rank (mpi_comm_world, mpi_rank, mpi_err)

  if (and(mpi_size, 1) /= 0) then
    print *, "This test requires an even number of ranks"
    call mpi_abort(mpi_comm_world, -1, mpi_err)
  end if
  if (and(mpi_rank, 1) /= 0) then
    mpi_peer = mpi_rank-1
  else
    mpi_peer = mpi_rank+1
  end if

  if (mpi_rank==0) then
    print *, mpi_size, "ranks and", nthreads, "threads per rank"
  end if

  ! init oomph
  call oomph_init(nthreads, mpi_comm_world)

  !$omp parallel private(thrid, comm, sreq, rreq, smsg, rmsg, msg_data)

  ! make thread id 1-based
  thrid = omp_get_thread_num()+1

  ! allocate a communicator per thread
  comm = oomph_get_communicator()

  ! create a message per thread
  rmsg = oomph_message_new(msg_size, OomphAllocatorHost)
  smsg = oomph_message_new(msg_size, OomphAllocatorHost)
  msg_data => oomph_message_data(smsg)
  msg_data(1:msg_size) = int((mpi_rank+1)*nthreads + thrid, 1)

  ! send / recv with a request, tag 1
  call oomph_comm_post_send(comm, smsg, mpi_peer, thrid, sreq)
  call oomph_comm_post_recv(comm, rmsg, mpi_peer, thrid, rreq)

  ! wait for comm
  do while( .not.oomph_request_test(sreq) .or. .not.oomph_request_test(rreq) )
  end do

  ! what have we received?
  msg_data => oomph_message_data(rmsg)
  if (any(msg_data /= (mpi_peer+1)*nthreads + thrid)) then
    print *, "wrong data received"
    print *, mpi_rank, ": ", thrid, ": ", msg_data
    call exit(1)
  end if

  ! cleanup per-thread
  call oomph_free(rmsg)
  call oomph_free(smsg)
  call oomph_free(comm)

  !$omp end parallel

  if (mpi_rank == 0) then
    print *, "result: OK"
  end if
  call oomph_finalize()  
  call mpi_finalize(mpi_err)

END PROGRAM test_send_recv_ft
