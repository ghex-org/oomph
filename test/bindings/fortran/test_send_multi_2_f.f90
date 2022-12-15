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

  ! a data structure passed by GHEX to the recv completion callback
  ! In the callback we count the number of received messages (from each rank)
  ! and resubmit the recv request. We store the req handle for that request,
  ! which is needed to cancel it at the end of the run.
  type :: mesg_info
     integer, dimension(:), pointer   :: completed   ! counter for completed messages (0:mpi_size-1)
     type(oomph_request), dimension(:), pointer :: reqs        ! recv request handles for canceling at the end (0:mpi_size-1)
  end type mesg_info

  integer :: mpi_err
  integer :: mpi_threading, mpi_size, mpi_rank
  integer :: nthreads = 0
  integer(8) :: msg_size = 16

  ! shared array to store per-thread communicators (size 0-nthreads-1)
  type(oomph_communicator), dimension(:), pointer :: communicators

  ! recv data structures (size 0-mpi_size-1):
  !  - recv requests to be able to cancel outstanding comm
  !  - shared array to count messages received from each peer rank
  !  - an array of recv messages
  type(oomph_message), dimension(:), pointer :: rmesg
  type(oomph_cb_user_data) :: user_data
  type(mesg_info), volatile, target :: minfo

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

  if (mpi_rank==0) then
    print *, mpi_size, "ranks and", nthreads, "threads per rank"
  end if

  ! init oomph
  call oomph_init(nthreads, mpi_comm_world)


  ! ---------------------------------------------------------
  ! allocate a communicator per thread and store in a shared array
  ! ---------------------------------------------------------
  allocate(communicators(nthreads))
  !$omp parallel private(it, thrid, peer, peers, comm, status, smsg, msg_data, rreq, sreq, user_data)
  thrid = omp_get_thread_num()+1
  communicators(thrid) = oomph_get_communicator()
  !$omp end parallel


  ! ---------------------------------------------------------
  ! submit recv requests - one per peer (mpi_size-1).
  ! ---------------------------------------------------------
  comm = communicators(1)

  ! Submit the recv requests - 1 per peer rank.
  ! Requests are resubmitted in the callback.
  ! This is done outside of OpenMP parallel region.
  allocate (rmesg(0:mpi_size-1))
  allocate (minfo%completed(0:mpi_size-1))
  allocate (minfo%reqs(0:mpi_size-1))

  ! this HAS TO BE before the OpenMP block
  ! Intel MPI breaks everything if this is done there.
  ! In short, there seems to be some problem with thread local storage.
  ! Global variables cannot be written to by threads,
  ! as this causes some variable address issues.
  pcb => recv_callback

  ! data passed in the callback, together with the received message
  minfo%completed = 0
  user_data%data = c_loc(minfo)

  ! pre-post a recv
  it = 0
  do while (it<mpi_size)
    if (it/=mpi_rank) then
      rmesg(it) = oomph_message_new(msg_size, OomphAllocatorHost)
      call oomph_comm_recv_cb(comm, rmesg(it), it, it, pcb, user_data=user_data, req=rreq)
      minfo%reqs(it) = rreq
    end if
    it = it+1
  end do

  call oomph_barrier(OomphBarrierRank)
  if (mpi_rank==0) then
    print *, "starting..."
  end if

  !$omp parallel private(it, thrid, peer, peers, comm, status, smsg, msg_data, rreq, sreq, user_data)
  thrid = omp_get_thread_num()+1
  comm = communicators(thrid)

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
  msg_data(1:msg_size) = int((mpi_rank+1)*nthreads + thrid, 1)

  ! ! send without a callback (returns a request), keep ownership of the message
  ! call oomph_comm_post_send_multi(comm, smsg, peers, mpi_rank, req=sreq)

  ! ! send with a callback (can be empty), keep ownership of the message
  ! call oomph_comm_post_send_multi_cb(comm, smsg, peers, mpi_rank, req=sreq)

  ! send with a callback (can be empty), give ownership of the message to oomph: smsg buffer will be freed after completion
  call oomph_comm_send_multi_cb(comm, smsg, peers, mpi_rank)

  ! wait for all recv requests to complete - enough if only master does this,
  ! the other threads also have to progress the communication, but that happens
  ! in the call to oomph_comm_barrier below
  !$omp single
  it   = 0
  do while(it < mpi_size)
    if (it /= mpi_rank) then
      do while (minfo%completed(it) /= nthreads)
        call oomph_comm_progress(comm)
      end do
      print *, mpi_rank, "received", minfo%completed(it), "messages from rank", it
    end if
    it = it+1
  end do
  ! OBS: nowait is required to avoid the implicit barrier. OpenMP barriers cannot be used.
  ! Instead, one must use oomph barriers to progress the communication.
  !$omp end single nowait

  ! OBS: has to be a global (threads + ranks) barrier before permanently leaving the OpenMP region
  ! This assures that all prior communication has been completed, and no threads will hang waiting.
  call oomph_barrier(OomphBarrierGlobal)

  ! cancel all outstanding recv requests
  !$omp master
  it   = 0
  do while(it < mpi_size)
    if (it /= mpi_rank) then
      if (.not.oomph_request_ready(minfo%reqs(it))) then
        status = oomph_request_cancel(minfo%reqs(it))
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
  deallocate (rmesg)
  deallocate (communicators)
  deallocate (minfo%reqs)
  deallocate (minfo%completed)

  call oomph_finalize()
  call mpi_finalize(mpi_err)

CONTAINS

  subroutine recv_callback (mesg, rank, tag, user_data) bind(c)
    use iso_c_binding
    type(oomph_message), value :: mesg
    integer(c_int), value :: rank, tag
    type(oomph_cb_user_data), value :: user_data
    type(mesg_info), pointer :: minfo

    type(oomph_request) :: rreq
    integer :: thrid

    ! NOTE: this segfaults in Intel compiler. It seems we have to use
    ! the globally defined pcb from the main function. WHY??
    ! procedure(f_callback), pointer :: pcb
    ! pcb => recv_callback

    ! needed to know which communicator we can use. Communicators are bound to threads.
    thrid = omp_get_thread_num()+1

    ! mark receipt in the user data
    ! OBS: this array is now 1-based, not 0-based as the original
    call c_f_pointer(user_data%data, minfo)

    ! atomic not needed now since only the master thread completes the recv requests
    !! $omp atomic
    minfo%completed(tag) = minfo%completed(tag)+1
    !! $omp end atomic

    ! resubmit
    call oomph_comm_resubmit_recv(communicators(thrid), mesg, rank, tag, pcb, rreq, user_data=user_data)

    ! cannot test the request: progress cannot be called inside the callback
    if (.not.oomph_request_ready(rreq)) then
      ! Due to immediate completion it can happen that the old request has not completed yet.
      ! The below sequence sorts this out: we only need to store the most recent incomplete request
      ! to cancel it at the end of the program. In reality this should be solved better.
      ! if (.not.oomph_request_ready(rrequests(tag))) then
      !   print *, mpi_rank, rank, thrid, "current request still not completed."
      ! end if
      minfo%reqs(tag) = rreq
    end if

  end subroutine recv_callback

END PROGRAM test_send_multi
