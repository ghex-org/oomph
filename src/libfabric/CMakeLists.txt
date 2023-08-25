find_package(Boost REQUIRED COMPONENTS thread)
add_library(oomph_private_libfabric_headers INTERFACE)
target_include_directories(oomph_private_libfabric_headers INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")
target_link_libraries(oomph_libfabric PRIVATE oomph_private_libfabric_headers)
target_link_libraries(oomph_libfabric PRIVATE Boost::thread)

list(TRANSFORM oomph_sources PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/../
    OUTPUT_VARIABLE oomph_sources_libfabric)
target_sources(oomph_libfabric PRIVATE ${oomph_sources_libfabric})
target_sources(oomph_libfabric PRIVATE context.cpp)
target_sources(oomph_libfabric PRIVATE operation_context.cpp)
target_sources(oomph_libfabric PRIVATE locality.cpp)
