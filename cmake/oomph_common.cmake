# ---------------------------------------------------------------------
# global implementation detail options
# ---------------------------------------------------------------------
set(OOMPH_USE_FAST_PIMPL OFF CACHE BOOL "store private implementations on stack")
mark_as_advanced(OOMPH_USE_FAST_PIMPL)
if (OOMPH_USE_FAST_PIMPL)
    target_compile_definitions(oomph INTERFACE OOMPH_USE_FAST_PIMPL)
endif()

# ---------------------------------------------------------------------
# compiler and linker flags
# ---------------------------------------------------------------------
function(oomph_target_compile_options target)
    set_target_properties(${target} PROPERTIES INTERFACE_POSITION_INDEPENDENT_CODE ON)
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
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
