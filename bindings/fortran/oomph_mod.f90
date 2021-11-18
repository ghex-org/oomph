!
! ghex-org
!
! Copyright (c) 2014-2021, ETH Zurich
! All rights reserved.
!
! Please, refer to the LICENSE file in the root directory.
! SPDX-License-Identifier: BSD-3-Clause
!
MODULE oomph_mod
    use iso_c_binding
    use oomph_defs
    !use ghex_comm_mod
    !use ghex_message_mod
    !use ghex_structured_mod
    !use ghex_cubed_sphere_mod
    
    implicit none
    
    interface
        subroutine oomph_init(mpi_comm) bind(c)
            use iso_c_binding
            integer, value :: mpi_comm
        end subroutine oomph_init

        subroutine oomph_finalize() bind(c)
            use iso_c_binding
        end subroutine oomph_finalize
    end interface

END MODULE oomph_mod
