PROGRAM test_send_recv_cb
  use mpi
  use iso_fortran_env
  use omp_lib
  use oomph_mod
  use oomph_communicator_mod
  use oomph_message_mod
  use oomph_request_mod

  implicit none  

  integer :: mpi_err
  integer :: mpi_threading, mpi_size, mpi_rank, mpi_peer
  integer :: nthreads = 0, thrid

  type(oomph_communicator), dimension(:), pointer :: communicators
  type(oomph_communicator) :: comm

  ! message
  integer(8) :: msg_size = 16
  type(oomph_message) :: smsg, rmsg
  type(oomph_request) :: sreq
  type(oomph_cb_user_data) :: user_data
  integer, volatile, target :: tag_received
  integer(1), dimension(:), pointer :: msg_data
  procedure(f_callback), pointer :: pcb

  ! this HAS TO BE before the OpenMP block
  ! Intel MPI breaks everything if this is done there.
  ! In short, there seems to be some problem with thread local storage.
  ! Global variables cannot be written to by threads, 
  ! as this causes some variable address issues.
  pcb => recv_callback

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

  ! initialize shared datastructures
  allocate(communicators(nthreads))

  !$omp parallel private(thrid, comm, sreq, smsg, rmsg, msg_data, user_data, tag_received)

  ! make thread id 1-based
  thrid = omp_get_thread_num()+1

  ! allocate a communicator per thread and store in a shared array
  communicators(thrid) = oomph_get_communicator()
  comm = communicators(thrid)

  ! create messages
  rmsg = oomph_message_new(msg_size, OomphAllocatorHost)
  smsg = oomph_message_new(msg_size, OomphAllocatorHost)

  ! initialize send data
  msg_data => oomph_message_data(smsg)
  msg_data(1:msg_size) = int((mpi_rank+1)*nthreads, 1)

  ! pre-post a recv. subsequent recv are posted inside the callback.
  ! rmsg can not be accessed after this point (use post_recv_cb variant
  ! to keep ownership of the message)
  ! user_data is used to count completed receive requests. 
  ! This per-thread integer is updated inside the callback by any thread.
  ! Lock is not needed, because only one thread can write to it at the same time
  ! (recv requests are submitted sequentially, after completion)
  tag_received = 0
  user_data%data = c_loc(tag_received)
  call oomph_comm_recv_cb(comm, rmsg, mpi_peer, thrid, pcb, user_data = user_data)

  ! send, but keep ownership of the message: buffer is not freed after send
  call oomph_comm_post_send_cb(comm, smsg, mpi_peer, thrid, req=sreq)
  msg_data => oomph_message_data(smsg)

  ! progress the communication - complete the send before posting another one
  ! here we send the same buffer twice
  call oomph_request_wait(sreq)

  ! if overlapping is needed, test and progress instead of waiting
  ! do while(.not.oomph_request_test(sreq))
  !   call oomph_comm_progress(comm)
  ! end do

  ! send again, give ownership of the message to oomph: buffer will be freed automatically after completion
  call oomph_comm_send_cb(comm, smsg, mpi_peer, thrid)

  ! progress the communication - wait for all (2) recv to complete
  do while(tag_received/=2)
    call oomph_comm_progress(comm)
  end do

  ! wait for all while progressing the communication
  call oomph_barrier(OomphBarrierGlobal)

  ! cleanup per-thread. messages are freed by oomph if comm_recv_cb and comm_send_cb
  call oomph_free(comm)
  call oomph_free(rmsg)
  ! smsg has been freed by oomph after send completion
  ! call oomph_free(smsg)

  !$omp end parallel

  ! cleanup shared
  if (mpi_rank == 0) then
    print *, "result: OK"
  end if
  deallocate(communicators)
  call oomph_finalize()  
  call mpi_finalize(mpi_err)

contains

  ! --------------------
  ! In Intel compiler, this function CANNOT write to any global variables.
  ! If this is done in an OpenMP application, something goes wrong with
  ! variable address calculations / relocation.
  ! --------------------
  subroutine recv_callback (mesg, rank, tag, user_data) bind(c)
    use iso_c_binding   
    type(oomph_message), value :: mesg
    integer(c_int), value :: rank, tag
    type(oomph_cb_user_data), value :: user_data

    ! local variables
    integer :: thrid
    integer(1), dimension(:), pointer :: msg_data
    integer, pointer :: received

    ! NOTE: this segfaults in Intel compiler. It seems we have to use
    ! the globally defined pcb from the main function.
    ! procedure(f_callback), pointer :: pcb
    ! pcb => recv_callback

    ! needed to know which communicator we can use. Communicators are bound to threads.
    thrid = omp_get_thread_num()+1

    ! what have we received?
    msg_data => oomph_message_data(mesg)
    if (any(msg_data /= (rank+1)*nthreads)) then
      print *, "wrong data received, expected", rank
      print *, mpi_rank, "received", msg_data
      call exit(1)
    end if

    ! mark receipt in the user data
    call c_f_pointer(user_data%data, received)
    received = received + 1

    ! resubmit if needed. here: receive only 2 (rank,tag) messages
    if (received < 2) then
      call oomph_comm_resubmit_recv(communicators(thrid), mesg, rank, tag, pcb, user_data = user_data)
    end if
  end subroutine recv_callback

END PROGRAM test_send_recv_cb
