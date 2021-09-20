# set all libfabric related options and values

#------------------------------------------------------------------------------
# Enable libfabric support
#------------------------------------------------------------------------------
set(OOMPH_WITH_LIBFABRIC OFF CACHE BOOL "Build with LIBFABRIC backend")

if (OOMPH_WITH_LIBFABRIC)
    find_package(Libfabric REQUIRED)
    add_library(oomph_libfabric SHARED)
    add_library(oomph::libfabric ALIAS oomph_libfabric)
    oomph_shared_lib_options(oomph_libfabric)
    target_link_libraries(oomph_libfabric PUBLIC libfabric::libfabric)
    install(TARGETS oomph_libfabric
        EXPORT oomph-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

    if (6 GREATER OOMPH_TRANSPORT_RESERVED_SPACE)
        set(OOMPH_TRANSPORT_RESERVED_SPACE 6)
    endif()
endif()


