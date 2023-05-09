include(git_submodule)
include(external_project)

if(OOMPH_GIT_SUBMODULE)
    update_git_submodules()
endif()

cmake_dependent_option(OOMPH_USE_BUNDLED_HWMALLOC "Use bundled hwmalloc lib." ON
    "OOMPH_USE_BUNDLED_LIBS" OFF)
if(OOMPH_USE_BUNDLED_HWMALLOC)
    check_git_submodule(hwmalloc ext/hwmalloc)
    #set(WITH_FUNCTION_X OFF CACHE BOOL "enable X functionality" FORCE)
    add_subdirectory(ext/hwmalloc)
    add_library(HWMALLOC::hwmalloc ALIAS hwmalloc)
else()
    find_package(HWMALLOC REQUIRED)
endif()

# Set up google test as an external project
add_external_cmake_project(
    NAME googletest
    PATH ext/googletest
    INTERFACE_NAME ext-gtest
    LIBS libgtest.a libgtest_main.a
    CMAKE_ARGS
        "-DCMAKE_BUILD_TYPE=release"
        "-DBUILD_SHARED_LIBS=OFF"
        "-DBUILD_GMOCK=OFF")

# on some systems we need link explicitly against threads
if (TARGET ext-gtest)
    find_package (Threads)
    target_link_libraries(ext-gtest INTERFACE Threads::Threads)
endif()
