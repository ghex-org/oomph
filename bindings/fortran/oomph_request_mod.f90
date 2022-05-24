MODULE oomph_request_mod
  use iso_c_binding

  ! should be established and defined by cmake, also for request_bind.hpp
  implicit none
#include <bindings/fortran/oomph_sizes.hpp>

  type, bind(c) :: oomph_request
     integer(c_int8_t) :: data(OOMPH_REQUEST_SIZE) = 0
     logical(c_bool) :: recv_request
  end type oomph_request

  interface

     logical(c_bool) function oomph_request_test(req) bind(c)
       use iso_c_binding
       import oomph_request
       type(oomph_request) :: req
     end function oomph_request_test

     logical(c_bool) function oomph_request_ready(req) bind(c)
       use iso_c_binding
       import oomph_request
       type(oomph_request) :: req
     end function oomph_request_ready

     subroutine oomph_request_wait(req) bind(c)
       use iso_c_binding
       import oomph_request
       type(oomph_request) :: req
     end subroutine oomph_request_wait

     logical(c_bool) function oomph_request_cancel(req) bind(c)
       use iso_c_binding
       import oomph_request
       type(oomph_request) :: req
     end function oomph_request_cancel

  end interface

CONTAINS

  subroutine oomph_request_init(req)
    type(oomph_request) :: req
    req%recv_request = .false.
    req%data = 0
  end subroutine oomph_request_init

END MODULE oomph_request_mod
