cmake_minimum_required(VERSION 3.17)
# CMake version is set at 3.17 because of find_package(CUDAToolkit)
project(oomph VERSION 0.1 LANGUAGES CXX)

# ---------------------------------------------------------------------
# CMake setup, C++ version, build type, modules, etc
# ---------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE) # AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
    set_property(CACHE CMAKE_CXX_FLAGS PROPERTY VALUE "-Wall -Wextra -Wpedantic")
endif()

set(OOMPH_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${OOMPH_MODULE_PATH}")

# ---------------------------------------------------------------------
# Define main oomph library
# ---------------------------------------------------------------------
add_library(oomph INTERFACE)
#add_library(OOMPH::oomph ALIAS oomph)
#set_target_properties(oomph PROPERTIES INTERFACE_POSITION_INDEPENDENT_CODE ON)
include(oomph_common)

# ---------------------------------------------------------------------
# MPI setup
# ---------------------------------------------------------------------
find_package(MPI REQUIRED COMPONENTS CXX)
target_link_libraries(oomph INTERFACE MPI::MPI_CXX)

# ---------------------------------------------------------------------
# HWMALLOC setup
# ---------------------------------------------------------------------
include(hwmalloc)
target_link_libraries(oomph INTERFACE hwmalloc)

# ---------------------------------------------------------------------
# Boost setup
# ---------------------------------------------------------------------
find_package(Boost REQUIRED)
target_link_libraries(oomph INTERFACE Boost::boost)

# ---------------------------------------------------------------------
# include paths
# ---------------------------------------------------------------------
target_include_directories(hwmalloc INTERFACE
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>)

# ---------------------------------------------------------------------
# oomph MPI variant
# ---------------------------------------------------------------------
include(oomph_mpi)

# ---------------------------------------------------------------------
# oomph UCX variant
# ---------------------------------------------------------------------
include(oomph_ucx)

# ---------------------------------------------------------------------
# main src subdir
# ---------------------------------------------------------------------
add_subdirectory(src)

# ---------------------------------------------------------------------
# testing
# ---------------------------------------------------------------------
set(OOMPH_WITH_TESTING OFF CACHE BOOL "True if tests shall be built")
if (OOMPH_WITH_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()


