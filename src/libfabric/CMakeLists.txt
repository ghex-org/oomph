include(FetchContent)

# ------------------------------------------------------------------------------
#
# ------------------------------------------------------------------------------
if(EXISTS ${PROJECT_SOURCE_DIR}/ext/libfatbat)
  set(FETCHCONTENT_SOURCE_DIR_LIBFATBAT ${PROJECT_SOURCE_DIR}/ext/libfatbat)
endif()
fetchcontent_declare(
  libFatbat GIT_REPOSITORY "https://github.com/biddisco/libfatbat.git"
  GIT_TAG "develop"
)
fetchcontent_makeavailable(libFatbat)
message(
  STATUS
    "✅ libFatbat building: (${libFatbat_VERSION}) in ${libFatbat_BINARY_DIR}"
)

# ------------------------------------------------------------------------------
# spdlog logging library
# ------------------------------------------------------------------------------
string(TOUPPER ${FATBAT_LOG_LEVEL} FATBAT_LOG_LEVEL_UPPERCASE)
if(NOT "${FATBAT_LOG_LEVEL_UPPERCASE}" MATCHES "OFF")
  find_package(spdlog QUIET)
  if(spdlog_FOUND)
    message(STATUS "✅ spdlog found ${spdlog_VERSION}")
    message(STATUS "spdlog include dirs: ${spdlog_INCLUDE_DIRS}")
  else()
    message("Fetching spdlog")
    include(FetchContent)
    fetchcontent_declare(
      spdlog GIT_REPOSITORY https://github.com/gabime/spdlog.git GIT_TAG v2.x
    )
    fetchcontent_makeavailable(spdlog)
    message(STATUS "✅ spdlog found ${spdlog_VERSION}")
  endif()
endif()

# ------------------------------------------------------------------------------
# interface/dummy library of our private headers
# ------------------------------------------------------------------------------
add_library(oomph_private_libfabric_headers INTERFACE)
target_include_directories(
  oomph_private_libfabric_headers
  INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
)

# ------------------------------------------------------------------------------
# actual library (created in oomph_libfabric.cmake) source files, depends on dummy library
# ------------------------------------------------------------------------------
target_link_libraries(oomph_libfabric PRIVATE oomph_private_libfabric_headers)
target_link_libraries(oomph_libfabric PRIVATE libfatbat)

# we need to include a binary dir for the oomph_config_defines.hpp file
target_include_directories(
  oomph_libfabric
  INTERFACE "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/src/libfabric>"
)

list(TRANSFORM oomph_sources PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/../
                                     OUTPUT_VARIABLE oomph_sources_libfabric
)
target_sources(oomph_libfabric PRIVATE ${oomph_sources_libfabric})
target_sources(oomph_libfabric PRIVATE context.cpp)
target_sources(oomph_libfabric PRIVATE operation_context.cpp)

if(NOT "${FATBAT_LOG_LEVEL_UPPERCASE}" MATCHES "OFF")
  target_compile_definitions(
    oomph_libfabric INTERFACE LIBFATBAT_LOGGING_ENABLED=1
  )
  target_compile_definitions(
    oomph_libfabric
    INTERFACE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_${FATBAT_LOG_LEVEL_UPPERCASE}
  )

  target_link_libraries(
    oomph_libfabric INTERFACE $<BUILD_INTERFACE:spdlog::spdlog>
                              $<BUILD_INTERFACE:fmt::fmt>
  )
endif()

# if we are using GPU, then the libfabric library was probably built with gpu
# support, and we should link to cuda to prevent link errors
if(HWMALLOC_ENABLE_DEVICE)
  include(CheckLanguage)
  check_language(CUDA)

  if(CMAKE_CUDA_COMPILER)
    enable_language(CUDA)
  else()
    message(STATUS "No CUDA support")
    return()
  endif()

  find_package(CUDAToolkit)
  target_link_libraries(oomph_libfabric PRIVATE CUDA::cudart)
endif()

add_executable(check_libfabric test/check_libfabric.cpp)
target_link_libraries(check_libfabric PUBLIC oomph_libfabric)
target_link_libraries(check_libfabric PRIVATE libfatbat)
target_include_directories(check_libfabric PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
