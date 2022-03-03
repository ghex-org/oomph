!
! ghex-org
!
! Copyright (c) 2014-2021, ETH Zurich
! All rights reserved.
!
! Please, refer to the LICENSE file in the root directory.
! SPDX-License-Identifier: BSD-3-Clause
!
MODULE oomph_communicator_mod
  use iso_c_binding
  use oomph_message_mod
  use oomph_request_mod

  implicit none

  ! Communicator can return a request handle to track the progress.
  ! requests are returned by the callback-based API

  ! ---------------------
  ! --- module types
  ! ---------------------
  type, bind(c) :: oomph_communicator
     type(c_ptr) :: ptr = c_null_ptr
  end type oomph_communicator

  type, bind(c) :: oomph_cb_user_data
     type(c_ptr) :: data
  end type oomph_cb_user_data

  ! ---------------------
  ! --- module C interfaces
  ! ---------------------
  interface

     ! callback type
     subroutine f_callback (message, rank, tag, user_data) bind(c)
       use iso_c_binding
       import oomph_message, oomph_cb_user_data
       type(oomph_message), value :: message
       integer(c_int), value :: rank, tag
       type(oomph_cb_user_data), value :: user_data
     end subroutine f_callback

     type(oomph_communicator) function oomph_get_communicator() bind(c)
       use iso_c_binding
       import oomph_communicator
     end function oomph_get_communicator

     integer(c_int) function oomph_comm_rank(comm) bind(c)
       use iso_c_binding
       import oomph_communicator
       type(oomph_communicator), value :: comm
     end function oomph_comm_rank

     integer(c_int) function oomph_comm_size(comm) bind(c)
       use iso_c_binding
       import oomph_communicator
       type(oomph_communicator), value :: comm
     end function oomph_comm_size

     subroutine oomph_comm_progress(comm) bind(c)
       use iso_c_binding
       import oomph_communicator
       type(oomph_communicator), value :: comm
     end subroutine oomph_comm_progress

     ! -----------------------------------------------------------------------------------------
     ! SEND requests
     ! -----------------------------------------------------------------------------------------

     ! post a send on a message: message is still owned by the user
     ! and has to be freed when necessary
     ! request has to be tested / waited on to assure completion
     subroutine oomph_comm_post_send(comm, message, rank, tag, request) bind(c)
       use iso_c_binding
       import oomph_communicator, oomph_message, oomph_request
       type(oomph_communicator), value :: comm
       type(oomph_message), value :: message
       integer(c_int), value :: rank
       integer(c_int), value :: tag
       type(oomph_request) :: request
     end subroutine oomph_comm_post_send

     ! WRAPPED - you should call oomph_comm_post_send_cb
     ! post a send on a message: message is still owned by the user
     ! and has to be freed when necessary
     ! callback is called upon completion of the communication request
     ! request (optional) is returned and can be tested for completion, but not waited on
     subroutine oomph_comm_post_send_cb_wrapped(comm, message, rank, tag, cb, req, user_data) bind(c)
       use iso_c_binding
       import oomph_communicator, oomph_message, oomph_request, oomph_cb_user_data
       type(oomph_communicator), value :: comm
       type(oomph_message), value :: message
       integer(c_int), value :: rank
       integer(c_int), value :: tag
       type(c_funptr), value :: cb
       type(oomph_request) :: req
       type(oomph_cb_user_data), value :: user_data
     end subroutine oomph_comm_post_send_cb_wrapped

     ! WRAPPED - you should call oomph_comm_send_cb
     ! send a message with callback:
     ! message is taken over by oomph, and users copy is freed
     ! callback is called upon completion of the communication request
     ! request (optional) is returned and can be tested for completion, but not waited on
     subroutine oomph_comm_send_cb_wrapped(comm, message, rank, tag, cb, req, user_data) bind(c)
       use iso_c_binding
       import oomph_communicator, oomph_message, oomph_request, oomph_cb_user_data
       type(oomph_communicator), value :: comm
       type(oomph_message) :: message
       integer(c_int), value :: rank
       integer(c_int), value :: tag
       type(c_funptr), value :: cb
       type(oomph_request) :: req
       type(oomph_cb_user_data), value :: user_data
     end subroutine oomph_comm_send_cb_wrapped


     ! -----------------------------------------------------------------------------------------
     ! RECV requests
     ! -----------------------------------------------------------------------------------------

     ! post a recv on a message: message is still owned by the user
     ! and has to be freed when necessary
     ! request has to be tested / waited on to assure completion
     subroutine oomph_comm_post_recv(comm, message, rank, tag, request) bind(c)
       use iso_c_binding
       import oomph_communicator, oomph_message, oomph_request
       type(oomph_communicator), value :: comm
       type(oomph_message), value :: message
       integer(c_int), value :: rank
       integer(c_int), value :: tag
       type(oomph_request) :: request
     end subroutine oomph_comm_post_recv

     ! WRAPPED - you should call oomph_comm_post_recv_cb
     ! post a recv on a message: message is still owned by the user
     ! and has to be freed when necessary
     ! callback is called upon completion of the communication request
     ! request (optional) is returned and can be tested for completion, but not waited on
     subroutine oomph_comm_post_recv_cb_wrapped(comm, message, rank, tag, cb, req, user_data) bind(c)
       use iso_c_binding
       import oomph_communicator, oomph_message, oomph_request, oomph_cb_user_data
       type(oomph_communicator), value :: comm
       type(oomph_message), value :: message
       integer(c_int), value :: rank
       integer(c_int), value :: tag
       type(c_funptr), value :: cb
       type(oomph_request) :: req
       type(oomph_cb_user_data), value :: user_data
     end subroutine oomph_comm_post_recv_cb_wrapped

     ! WRAPPED - you should call oomph_comm_recv_cb_wrapped
     ! recv a message with callback:
     ! message is taken over by oomph, and users copy is freed
     ! callback is called upon completion of the communication request
     ! request (optional) is returned and can be tested for completion, but not waited on
     subroutine oomph_comm_recv_cb_wrapped(comm, message, rank, tag, cb, req, user_data) bind(c)
       use iso_c_binding
       import oomph_communicator, oomph_message, oomph_request, oomph_cb_user_data
       type(oomph_communicator), value :: comm
       type(oomph_message) :: message
       integer(c_int), value :: rank
       integer(c_int), value :: tag
       type(c_funptr), value :: cb
       type(oomph_request) :: req
       type(oomph_cb_user_data), value :: user_data
     end subroutine oomph_comm_recv_cb_wrapped

     ! -----------------------------------------------------------------------------------------
     ! resubmission of recv requests from inside callbacks        
     ! -----------------------------------------------------------------------------------------

     ! WRAPPED - you should call oomph_comm_resubmit_recv
     ! resubmit a recv on a message inside a completion callback:
     ! callback is called upon completion of the communication request
     ! request (optional) is returned and can be tested for completion, but not waited on
     subroutine oomph_comm_resubmit_recv_wrapped(comm, message, rank, tag, cb, req, user_data) bind(c)
       use iso_c_binding
       import oomph_communicator, oomph_message, oomph_request, oomph_cb_user_data
       type(oomph_communicator), value :: comm
       type(oomph_message), value :: message
       integer(c_int), value :: rank
       integer(c_int), value :: tag
       type(c_funptr), value :: cb
       type(oomph_request) :: req
       type(oomph_cb_user_data), value :: user_data
     end subroutine oomph_comm_resubmit_recv_wrapped

  end interface


  ! ---------------------
  ! --- generic oomph interfaces
  ! ---------------------
  interface oomph_free
     subroutine oomph_comm_free(comm) bind(c, name="oomph_obj_free")
       use iso_c_binding
       import oomph_communicator
       type(oomph_communicator) :: comm
     end subroutine oomph_comm_free
  end interface oomph_free

CONTAINS

  ! Need the wrappers for send/recv to enforce correct callback type,
  ! and to handle optional arguments

  subroutine oomph_comm_post_send_cb(comm, message, rank, tag, cb, req, user_data)
    use iso_c_binding
    type(oomph_communicator), intent(in) :: comm
    type(oomph_message), value :: message
    integer, intent(in) :: rank
    integer, intent(in) :: tag
    procedure(f_callback), optional, pointer :: cb
    type(oomph_request), optional :: req
    type(oomph_cb_user_data), optional :: user_data

    ! local variables
    procedure(f_callback), pointer :: lcb
    type(oomph_request) :: lreq
    type(oomph_cb_user_data) :: luser_data

    ! This is needed for GCC. Otherwise c_funloc(cart_nbor) doesn't work correctly
    ! This is a difference wrt. Intel compiler
    if (present(cb)) then
      lcb => cb
    else
      lcb => null()
    end if

    if (present(user_data)) then
      luser_data = user_data
    end if

    call oomph_comm_post_send_cb_wrapped(comm, message, rank, tag, c_funloc(lcb), lreq, luser_data)

    if (present(req)) then
      req = lreq
    end if
  end subroutine oomph_comm_post_send_cb

  subroutine oomph_comm_send_cb(comm, message, rank, tag, cb, req, user_data)
    use iso_c_binding
    type(oomph_communicator), intent(in) :: comm
    type(oomph_message) :: message
    integer, intent(in) :: rank
    integer, intent(in) :: tag
    procedure(f_callback), optional, pointer :: cb
    type(oomph_request), optional :: req
    type(oomph_cb_user_data), optional :: user_data

    ! local variables
    procedure(f_callback), pointer :: lcb
    type(oomph_request) :: lreq
    type(oomph_cb_user_data) :: luser_data

    lcb => null()
    if (present(cb)) then
      lcb => cb
    end if

    if (present(user_data)) then
      luser_data = user_data
    end if

    call oomph_comm_send_cb_wrapped(comm, message, rank, tag, c_funloc(lcb), lreq, luser_data)

    if (present(req)) then
      req = lreq
    end if
  end subroutine oomph_comm_send_cb

  subroutine oomph_comm_post_recv_cb(comm, message, rank, tag, cb, req, user_data)
    use iso_c_binding
    type(oomph_communicator), intent(in) :: comm
    type(oomph_message), value :: message
    integer, intent(in) :: rank
    integer, intent(in) :: tag
    procedure(f_callback), pointer :: cb
    type(oomph_request), optional :: req
    type(oomph_cb_user_data), optional :: user_data

    ! local variables
    procedure(f_callback), pointer :: lcb
    type(oomph_request) :: lreq
    type(oomph_cb_user_data) :: luser_data

    lcb => cb

    if (present(user_data)) then
      luser_data = user_data
    end if

    call oomph_comm_post_recv_cb_wrapped(comm, message, rank, tag, c_funloc(lcb), lreq, luser_data)

    if (present(req)) then
      req = lreq
    end if
  end subroutine oomph_comm_post_recv_cb

  subroutine oomph_comm_recv_cb(comm, message, rank, tag, cb, req, user_data)
    use iso_c_binding
    type(oomph_communicator), intent(in) :: comm
    type(oomph_message) :: message
    integer, intent(in) :: rank
    integer, intent(in) :: tag
    procedure(f_callback), pointer :: cb
    type(oomph_request), optional :: req
    type(oomph_cb_user_data), optional :: user_data

    ! local variables
    procedure(f_callback), pointer :: lcb
    type(oomph_request) :: lreq
    type(oomph_cb_user_data) :: luser_data

    lcb => cb

    if (present(user_data)) then
      luser_data = user_data
    end if

    call oomph_comm_recv_cb_wrapped(comm, message, rank, tag, c_funloc(lcb), lreq, luser_data)

    if (present(req)) then
      req = lreq
    end if
  end subroutine oomph_comm_recv_cb

  subroutine oomph_comm_resubmit_recv(comm, message, rank, tag, cb, req, user_data)
    use iso_c_binding
    type(oomph_communicator), intent(in) :: comm
    type(oomph_message), value :: message
    integer, intent(in) :: rank
    integer, intent(in) :: tag
    procedure(f_callback), pointer :: cb
    type(oomph_request), optional :: req
    type(oomph_cb_user_data), optional :: user_data

    ! local variables
    procedure(f_callback), pointer :: lcb
    type(oomph_request) :: lreq
    type(oomph_cb_user_data) :: luser_data

    lcb => cb

    if (present(user_data)) then
      luser_data = user_data
    end if

    call oomph_comm_resubmit_recv_wrapped(comm, message, rank, tag, c_funloc(lcb), lreq, luser_data)

    if (present(req)) then
      req = lreq
    end if
  end subroutine oomph_comm_resubmit_recv

END MODULE oomph_communicator_mod
