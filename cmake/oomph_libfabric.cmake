# set all libfabric related options and values

#------------------------------------------------------------------------------
# Enable libfabric support
#------------------------------------------------------------------------------
set(OOMPH_WITH_LIBFABRIC OFF CACHE BOOL "Build with LIBFABRIC backend")

if (OOMPH_WITH_LIBFABRIC)
    find_package(Libfabric REQUIRED)
    add_library(oomph_libfabric SHARED)
    target_link_libraries(oomph_libfabric PUBLIC oomph)
    target_link_libraries(oomph_libfabric PUBLIC libfabric::libfabric)
endif()


