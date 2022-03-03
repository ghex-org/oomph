# ---------------------------------------------------------------------
# global implementation detail options
# ---------------------------------------------------------------------
set(OOMPH_USE_FAST_PIMPL OFF CACHE BOOL "store private implementations on stack")
set(OOMPH_ENABLE_BARRIER ON CACHE BOOL "enable thread barrier (disable for task based runtime)")
mark_as_advanced(OOMPH_USE_FAST_PIMPL)

# ---------------------------------------------------------------------
# compiler and linker flags
# ---------------------------------------------------------------------
set(cxx_lang "$<COMPILE_LANGUAGE:CXX>")
set(cxx_lang_clang "$<COMPILE_LANG_AND_ID:CXX,Clang>")
#set(cuda_lang "$<COMPILE_LANGUAGE:CUDA>")
set(fortran_lang "$<COMPILE_LANGUAGE:Fortran>")
set(fortran_lang_gnu "$<COMPILE_LANG_AND_ID:Fortran,GNU>")

function(oomph_target_compile_options target)
    set_target_properties(${target} PROPERTIES INTERFACE_POSITION_INDEPENDENT_CODE ON)
    #target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(${target} PRIVATE
    # flags for CXX builds
    $<${cxx_lang}:$<BUILD_INTERFACE:-Wall -Wextra -Wpedantic -Wno-unknown-pragmas -Wno-unused-local-typedef>>
    $<${cxx_lang_clang}:$<BUILD_INTERFACE:-Wno-c++17-extensions -Wno-unused-lambda-capture>>
    # flags for CUDA builds
    #$<${cuda_lang}:$<BUILD_INTERFACE:-Xcompiler=-Wall -Wextra -Wno-unknown-pragmas --default-stream per-thread>>
    # flags for Fortran builds
    $<${fortran_lang}:$<BUILD_INTERFACE:-cpp -fcoarray=single>>
    $<${fortran_lang_gnu}:$<BUILD_INTERFACE:-ffree-line-length-none>>)
    if (OOMPH_ENABLE_BARRIER)
        target_compile_definitions(${target} PRIVATE OOMPH_ENABLE_BARRIER)
    endif()
endfunction()

function(oomph_target_link_options target)
    target_link_libraries(${target} PUBLIC oomph)
    target_link_libraries(${target} PUBLIC oomph_common)
endfunction()

function(oomph_shared_lib_options target)
    oomph_target_compile_options(${target})
    oomph_target_link_options(${target})
endfunction()

# ---------------------------------------------------------------------
# common library (static): independent of backend
# ---------------------------------------------------------------------
add_library(oomph_common STATIC)
oomph_target_compile_options(oomph_common)
target_link_libraries(oomph_common PUBLIC oomph)

# ---------------------------------------------------------------------
# install rules
# ---------------------------------------------------------------------
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS oomph oomph_common
    EXPORT oomph-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

