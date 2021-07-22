# set all MPI related options and values

#------------------------------------------------------------------------------
# Enable MPI support
#------------------------------------------------------------------------------
set(OOMPH_WITH_MPI ON CACHE BOOL "Build with MPI backend")

if (OOMPH_WITH_MPI)
    add_library(oomph_mpi SHARED)
    add_library(oomph::mpi ALIAS oomph_mpi)
    oomph_shared_lib_options(oomph_mpi)
endif()

