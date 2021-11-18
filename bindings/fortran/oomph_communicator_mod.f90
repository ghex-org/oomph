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
    !use ghex_message_mod
    !use ghex_request_mod
    !use ghex_future_mod

    implicit none

    ! Communicator can return either a future, or a request handle to track the progress.
    ! requests are returned by the callback-based API
    ! futures are returned by the 

    ! ---------------------
    ! --- module types
    ! ---------------------
    type, bind(c) :: oomph_communicator
        type(c_ptr) :: ptr = c_null_ptr
    end type oomph_communicator

    ! ---------------------
    ! --- module C interfaces
    ! ---------------------
    interface

        type(oomph_communicator) function oomph_comm_new() bind(c, name="oomph_get_communicator")
            use iso_c_binding
            import oomph_communicator
        end function oomph_comm_new

    end interface

    
    ! ---------------------
    ! --- generic ghex interfaces
    ! ---------------------
    interface oomph_free
        subroutine oomph_comm_free(comm) bind(c, name="oomph_obj_free")
            use iso_c_binding
            import oomph_communicator
            type(oomph_communicator) :: comm
        end subroutine oomph_comm_free
    end interface oomph_free

END MODULE oomph_communicator_mod
