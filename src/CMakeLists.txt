
#add_library(oomph_common SHARED)
#target_link_libraries(oomph_common PUBLIC oomph)
target_sources(oomph_common PRIVATE barrier.cpp)

if (OOMPH_WITH_MPI)
    add_subdirectory(mpi)
    #target_link_libraries(oomph_mpi oomph_common)
endif()

if (OOMPH_WITH_UCX)
    add_subdirectory(ucx)
endif()

if (OOMPH_WITH_LIBFABRIC)
    #add_subdirectory(libfabric)
endif()
