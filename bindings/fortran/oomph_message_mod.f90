MODULE oomph_message_mod
  use iso_c_binding
  implicit none


  ! ---------------------
  ! --- module types
  ! ---------------------
  type, bind(c) :: oomph_message
     type(c_ptr) :: ptr = c_null_ptr
  end type oomph_message


  ! ---------------------
  ! --- module C interfaces
  ! ---------------------
  interface

     type(oomph_message) function oomph_message_new(size, allocator) bind(c)
       use iso_c_binding
       import oomph_message
       integer(c_size_t), value :: size
       integer(c_int), value :: allocator
     end function oomph_message_new

     subroutine oomph_message_zero(message) bind(c)
       use iso_c_binding
       import oomph_message
       type(oomph_message), value :: message
     end subroutine oomph_message_zero

     type(c_ptr) function oomph_message_data_wrapped(message, size) bind(c)
       use iso_c_binding
       import oomph_message
       type(oomph_message), value :: message
       integer(c_size_t), intent(out) :: size
     end function oomph_message_data_wrapped
  end interface


  ! ---------------------
  ! --- generic interfaces
  ! ---------------------
  interface oomph_free
     subroutine oomph_message_free(message) bind(c)
       use iso_c_binding
       import oomph_message
       ! reference, not a value - fortran variable is reset to null 
       type(oomph_message) :: message
     end subroutine oomph_message_free
  end interface oomph_free

CONTAINS

  function oomph_message_data(message)
    use iso_c_binding
    type(oomph_message), value :: message
    integer(1), dimension(:), pointer :: oomph_message_data

    type(c_ptr) :: c_data
    integer(c_size_t) :: size

    ! get the data pointer
    c_data = oomph_message_data_wrapped(message, size)
    call c_f_pointer(c_data, oomph_message_data, [size])
  end function oomph_message_data

END MODULE oomph_message_mod
