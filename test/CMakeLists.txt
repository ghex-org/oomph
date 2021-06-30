include(gtest)

add_subdirectory(mpi_runner)

if (OOMPH_WITH_MPI)
    add_subdirectory(mpi)
endif()

if (OOMPH_WITH_UCX)
    add_subdirectory(ucx)
endif()

if (OOMPH_WITH_LIBFABRIC)
    add_subdirectory(libfabric)
endif()
