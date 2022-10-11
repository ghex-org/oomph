add_library(oomph_private_mpi_headers INTERFACE)
target_include_directories(oomph_private_mpi_headers INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")
target_link_libraries(oomph_mpi PRIVATE oomph_private_mpi_headers)

list(TRANSFORM oomph_sources PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/../
    OUTPUT_VARIABLE oomph_sources_mpi)
target_sources(oomph_mpi PRIVATE ${oomph_sources_mpi})
target_sources(oomph_mpi PRIVATE context.cpp)
