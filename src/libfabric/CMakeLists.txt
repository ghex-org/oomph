find_package(Boost REQUIRED COMPONENTS thread)

target_sources(oomph_libfabric PRIVATE src.cpp)
target_include_directories(oomph_libfabric PUBLIC $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>)
target_link_libraries(oomph_libfabric PRIVATE Boost::thread)
