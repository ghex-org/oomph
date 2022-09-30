find_package(Boost REQUIRED COMPONENTS thread)
#find_package(CUDAToolkit REQUIRED)

target_sources(oomph_libfabric PRIVATE src.cpp)
target_include_directories(oomph_libfabric PUBLIC $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>/src/libfabric)
target_link_libraries(oomph_libfabric PRIVATE Boost::thread)
#target_link_libraries(oomph_libfabric PUBLIC CUDA::cudart CUDA::cuda_driver)
