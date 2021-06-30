# set all MPI related options and values

#------------------------------------------------------------------------------
# Enable MPI support
#------------------------------------------------------------------------------
set(OOMPH_WITH_MPI ON CACHE BOOL "Build with MPI backend")

if (OOMPH_WITH_MPI)
    add_library(oomph_mpi SHARED)
    target_link_libraries(oomph_mpi PUBLIC oomph)
    #target_compile_definitions(oomph_mpi PUBLIC OOMPH_TRANSPORT=mpi)
    #target_link_libraries(oomph_mpi PUBLIC MPI::MPI_CXX)
endif()

