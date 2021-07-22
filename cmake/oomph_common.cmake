
set_target_properties(oomph PROPERTIES INTERFACE_POSITION_INDEPENDENT_CODE ON)

set(OOMPH_USE_FAST_PIMPL OFF CACHE BOOL "store private implementations on stack")
mark_as_advanced(OOMPH_USE_FAST_PIMPL)

if (OOMPH_USE_FAST_PIMPL)
    target_compile_definitions(oomph INTERFACE OOMPH_USE_FAST_PIMPL)
endif()

add_library(oomph_common STATIC)
target_link_libraries(oomph_common PUBLIC oomph)
#set_target_properties(oomph_common PROPERTIES INTERFACE_POSITION_INDEPENDENT_CODE ON)
