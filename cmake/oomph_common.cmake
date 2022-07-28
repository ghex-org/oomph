# ---------------------------------------------------------------------
# global implementation detail options
# ---------------------------------------------------------------------
set(OOMPH_USE_FAST_PIMPL OFF CACHE BOOL "store private implementations on stack")
set(OOMPH_ENABLE_BARRIER ON CACHE BOOL "enable thread barrier (disable for task based runtime)")
set(OOMPH_RECURSION_DEPTH "20" CACHE STRING "Callback recursion depth")
mark_as_advanced(OOMPH_USE_FAST_PIMPL)

set(cxx_lang "$<COMPILE_LANGUAGE:CXX>")
set(cxx_lang_clang "$<COMPILE_LANG_AND_ID:CXX,Clang>")
set(fortran_lang "$<COMPILE_LANGUAGE:Fortran>")
set(fortran_lang_gnu "$<COMPILE_LANG_AND_ID:Fortran,GNU>")

# ---------------------------------------------------------------------
# compiler and linker flags
# ---------------------------------------------------------------------
function(oomph_target_compile_options target)
    set_target_properties(${target} PROPERTIES INTERFACE_POSITION_INDEPENDENT_CODE ON)
    target_compile_options(${target} PRIVATE
        $<${cxx_lang}:$<BUILD_INTERFACE:-Wall -Wextra -Wpedantic>>
        #$<${fortran_lang}:$<BUILD_INTERFACE:-cpp -fcoarray=single>>
        #$<${fortran_lang_gnu}:$<BUILD_INTERFACE:-ffree-line-length-none>
    )
    target_compile_definitions(${target} PRIVATE
        OOMPH_RECURSION_DEPTH=${OOMPH_RECURSION_DEPTH})
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

