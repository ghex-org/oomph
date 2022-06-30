target_sources(oomph_common PRIVATE utils.cpp)
target_sources(oomph_common PRIVATE thread_id.cpp)
target_sources(oomph_common PRIVATE rank_topology.cpp)
target_sources(oomph_common PRIVATE env_vars.cpp)

if (OOMPH_WITH_MPI)
    add_subdirectory(mpi)
endif()

if (OOMPH_WITH_UCX)
    add_subdirectory(ucx)
endif()

if (OOMPH_WITH_LIBFABRIC)
    add_subdirectory(libfabric)
endif()
