PROGRAM test_message
  use mpi
  use oomph_mod

  implicit none

  integer(8) :: msg_size = 16
  integer(1) :: i
  integer :: mpi_err
  integer :: mpi_threading
  type(oomph_message) :: msg
  integer(1), dimension(:), pointer :: msg_data

  call mpi_init_thread (MPI_THREAD_SINGLE, mpi_threading, mpi_err)

  call oomph_init(1, MPI_COMM_WORLD)
  msg = oomph_message_new(msg_size, OomphAllocatorHost)

  msg_data => oomph_message_data(msg)
  msg_data(1:msg_size) = (/(i, i=1,int(msg_size, 1),1)/)

  print *, "values:    ", msg_data

  ! cleanup
  print *, "result: OK"
  call oomph_free(msg)
  call oomph_finalize()
  call mpi_finalize(mpi_err)

END PROGRAM test_message
