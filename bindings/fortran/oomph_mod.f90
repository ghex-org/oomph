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
  use oomph_communicator_mod
  use oomph_message_mod
  use oomph_request_mod

  implicit none

  interface
     subroutine oomph_init(nthreads, mpi_comm) bind(c)
       use iso_c_binding
       integer, value :: nthreads, mpi_comm
     end subroutine oomph_init

     subroutine oomph_finalize() bind(c)
       use iso_c_binding
     end subroutine oomph_finalize

#ifdef OOMPH_ENABLE_BARRIER
     subroutine oomph_barrier(type) bind(c)
       use iso_c_binding
       integer, value :: type
     end subroutine oomph_barrier
#endif
  end interface

END MODULE oomph_mod
