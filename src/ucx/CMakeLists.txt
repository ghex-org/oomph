add_library(oomph_private_ucx_headers INTERFACE)
target_include_directories(oomph_private_ucx_headers INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")
target_link_libraries(oomph_ucx PRIVATE oomph_private_ucx_headers)

list(TRANSFORM oomph_sources PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/../
    OUTPUT_VARIABLE oomph_sources_ucx)
target_sources(oomph_ucx PRIVATE ${oomph_sources_ucx})
target_sources(oomph_ucx PRIVATE context.cpp)
