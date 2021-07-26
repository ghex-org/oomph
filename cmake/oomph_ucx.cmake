# set all ucx related options and values

#------------------------------------------------------------------------------
# Enable ucx support
#------------------------------------------------------------------------------
set(OOMPH_WITH_UCX OFF CACHE BOOL "Build with UCX backend")

if (OOMPH_WITH_UCX)
    find_package(UCP REQUIRED)
    add_library(oomph_ucx SHARED)
    add_library(oomph::ucx ALIAS oomph_ucx)
    oomph_shared_lib_options(oomph_ucx)
    target_link_libraries(oomph_ucx PUBLIC UCP::libucp)
    install(TARGETS oomph_ucx
        EXPORT oomph-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

