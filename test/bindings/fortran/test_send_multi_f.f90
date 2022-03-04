PROGRAM test_send_multi

  ! This test starts m MPI ranks, each with n threads. Each thread sends the same message
  ! to all other ranks (using send_multi variant). So each rank sends n messages.
  ! Each rank pre-posts a single recv request to accept messages from all other ranks.
  ! After completion of a recv request, the receiving thread (i.e. thread, which calls
  ! the recv callback) re-submits the recv request.
  ! The test stops when each rank completes recv of n messages from each other rank.
  ! Then all the outstanding recv requests are canceled.

  use mpi
  use iso_fortran_env
  use omp_lib
  use oomph_mod

  implicit none

  integer :: mpi_err
  integer :: mpi_threading, mpi_size, mpi_rank, mpi_peer
  integer :: nthreads = 0
  integer(8) :: msg_size = 16

  ! shared array to store per-thread communicators (size 0-nthreads-1)
  type(oomph_communicator), dimension(:), pointer :: communicators

  ! recv data structures (size 0-mpi_size-1):
  !  - recv requests to be able to cancel outstanding comm
  !  - shared array to count messages received from each peer rank
  !  - an array of recv messages
  type(oomph_request), dimension(:), pointer :: rrequests
  integer, volatile,  dimension(:), pointer :: rank_received
  type(oomph_message), dimension(:), pointer :: rmsg
  type(oomph_cb_user_data) :: user_data

  ! thread-private data
  integer :: it, thrid, peer
  integer, dimension(:), pointer :: peers  ! MPI peer ranks to which to send a message
  type(oomph_communicator) :: comm
  logical :: status

  ! the sent message
  type(oomph_message) :: smsg
  integer(1), dimension(:), pointer :: msg_data
  type(oomph_request) :: sreq, rreq

  ! recv callback
  procedure(f_callback), pointer :: pcb

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

  ! init oomph
  call oomph_init(nthreads, mpi_comm_world);

  ! allocate shared data structures. things related to recv messages
  ! could be allocated here, but have to wait til per-thread communicator
  ! is created below.
  allocate(communicators(nthreads))

  !$omp parallel private(it, thrid, peer, peers, comm, status, smsg, msg_data, rreq, sreq)

  ! allocate a communicator per thread and store in a shared array
  thrid = omp_get_thread_num()+1
  communicators(thrid) = oomph_get_communicator()
  comm = communicators(thrid)

  ! these are recv requests - one per peer (mpi_size-1).
  ! could be done outside of the parallel block, but comm is required
  !$omp master

  ! initialize shared datastructures
  allocate(rrequests(0:mpi_size-1))
  allocate(rank_received(0:mpi_size-1))
  rank_received = 0

  ! pre-post a recv
  allocate (rmsg(0:mpi_size-1))
  it = 0
  user_data%data = c_loc(rank_received)
  pcb => recv_callback
  do while (it<mpi_size)
    if (it/=mpi_rank) then
      rmsg(it) = oomph_message_new(msg_size, OomphAllocatorHost)
      call oomph_comm_recv_cb(comm, rmsg(it), it, it, pcb, user_data=user_data, req=rreq)

      ! NOTE: we have to use a local rreq variable, because rrequests(it)
      ! can be overwritten in the cb routine. cb can be called below in oomph_comm_barrier.
      ! As a result, rrequests(it) is overwritten with a completed request
      ! by the above oomph_comm_recv_cb call.
      if (.not.oomph_request_test(rreq)) then
        rrequests(it) = rreq
      end if
    end if
    it = it+1
  end do
  !$omp end master

  ! wait for master to init the arrays
  call oomph_barrier(OomphBarrierThread)

  ! create list of peers (exclude self)
  allocate (peers(1:mpi_size-1))
  it   = 0
  peer = 1
  do while(it < mpi_size)
    if (it /= mpi_rank) then
      peers(peer) = it
      peer = peer+1
    end if
    it = it+1
  end do

  ! initialize send data
  smsg = oomph_message_new(msg_size, OomphAllocatorHost)
  msg_data => oomph_message_data(smsg)
  msg_data(1:msg_size) = (mpi_rank+1)*nthreads + thrid;

  ! send without a callback (returns a request), keep ownership of the message
  call oomph_comm_post_send_multi(comm, smsg, peers, mpi_rank, req=sreq)

  ! progress the communication - complete the send before posting another one
  call oomph_request_wait(sreq)

  ! send with a callback (can be empty), keep ownership of the message
  call oomph_comm_post_send_multi_cb(comm, smsg, peers, mpi_rank, req=sreq)

  ! progress the communication - complete the send before posting another one
  call oomph_request_wait(sreq)

  ! send with a callback (can be empty), give ownership of the message to oomph: smsg buffer will be freed after completion
  call oomph_comm_send_multi_cb(comm, smsg, peers, mpi_rank)

  ! wait for all recv requests to complete - enough if only master does this,
  ! the other threads also have to progress the communication, but that happens
  ! in the call to oomph_comm_barrier below
  !$omp master
  it   = 0
  do while(it < mpi_size)
    if (it /= mpi_rank) then
      do while (rank_received(it) /= nthreads*3)
        call oomph_comm_progress(comm)
      end do
      print *, mpi_rank, "received", rank_received(it), "messages from rank", it
    end if
    it = it+1
  end do
  !$omp end master

  ! wait for all threads and ranks to complete the recv.
  ! oomph_comm_barrier is safe as it progresses all communication: MPI and OOMPH
  call oomph_barrier(OomphBarrierGlobal)

  ! cancel all outstanding recv requests
  !$omp master
  it   = 0
  do while(it < mpi_size)
    if (it /= mpi_rank) then
      if (.not.oomph_request_ready(rrequests(it))) then
        status = oomph_request_cancel(rrequests(it))
        if (.not.status) then
          print *, "failed to cancel a recv request"
        end if
      end if
    end if
    it = it+1
  end do
  !$omp end master

  ! cleanup per-thread. messages are freed by oomph.
  deallocate (peers)
  call oomph_free(comm)

  !$omp end parallel

  ! cleanup shared
  deallocate (rmsg)
  deallocate (communicators)
  deallocate (rrequests)
  deallocate (rank_received)

  call oomph_finalize()
  call mpi_finalize(mpi_err)

CONTAINS

  subroutine recv_callback (mesg, rank, tag, user_data) bind(c)
    use iso_c_binding
    type(oomph_message), value :: mesg
    integer(c_int), value :: rank, tag
    type(oomph_cb_user_data), value :: user_data

    type(oomph_request) :: rreq
    integer :: thrid
    integer, volatile, dimension(:), pointer :: rank_received

    ! NOTE: this segfaults in Intel compiler. It seems we have to use
    ! the globally defined pcb from the main function. WHY??
    ! procedure(f_callback), pointer :: pcb
    ! pcb => recv_callback

    ! needed to know which communicator we can use. Communicators are bound to threads.
    thrid = omp_get_thread_num()+1

    ! mark receipt in the user data
    ! OBS: this array is now 1-based, not 0-based as the original
    call c_f_pointer(user_data%data, rank_received, [mpi_size])

    !$omp atomic
    rank_received(tag+1) = rank_received(tag+1)+1
    !$omp end atomic

    ! resubmit
    call oomph_comm_resubmit_recv(communicators(thrid), mesg, rank, tag, pcb, rreq, user_data=user_data)

    ! cannot test the request: progress cannot be called inside the callback
    if (.not.oomph_request_ready(rreq)) then
      ! if (.not.oomph_request_ready(rrequests(tag))) then
      !   print *, mpi_rank, rank, thrid, "current request still not completed. could be trouble."
      ! end if
      rrequests(tag) = rreq
    end if

  end subroutine recv_callback

END PROGRAM test_send_multi
