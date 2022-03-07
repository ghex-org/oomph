find_package(PkgConfig QUIET)
pkg_check_modules(PC_UCX QUIET ucx)

find_path(UCP_INCLUDE_DIR ucp/api/ucp.h
    HINTS
    ${HPCX_UCX_DIR}   ENV HPCX_UCX_DIR
    ${UCP_ROOT}  ENV UCX_ROOT
    ${UCP_DIR}   ENV UCX_DIR
    PATH_SUFFIXES include)

find_path(UCP_LIBRARY_DIR libucp.so
    HINTS
    ${HPCX_UCX_DIR}   ENV HPCX_UCX_DIR
    ${UCP_ROOT}  ENV UCX_ROOT
    ${UCP_DIR}   ENV UCX_DIR
    PATH_SUFFIXES lib lib64)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(UCP DEFAULT_MSG UCP_LIBRARY_DIR UCP_INCLUDE_DIR)
mark_as_advanced(UCP_LIBRARY_DIR UCP_INCLUDE_DIR)

if(NOT TARGET UCP::libucp AND UCP_FOUND)
  add_library(UCP::libucp INTERFACE IMPORTED)
  set_target_properties(UCP::libucp PROPERTIES
    INTERFACE_LINK_LIBRARIES "ucp;ucs;uct"
    INTERFACE_LINK_DIRECTORIES ${UCP_LIBRARY_DIR}
    INTERFACE_INCLUDE_DIRECTORIES ${UCP_INCLUDE_DIR}
  )
endif()
