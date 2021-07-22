add_library(gtest_main_mpi ./gtest_main_mpi.cpp)
target_link_libraries(gtest_main_mpi PRIVATE GTest::gtest)
target_link_libraries(gtest_main_mpi PRIVATE MPI::MPI_CXX)

