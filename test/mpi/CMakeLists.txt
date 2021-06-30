
function(reg_test_mpi t n)
    add_executable(${t} ${t}.cpp)
    target_link_libraries(${t} PRIVATE gtest_main_mpi)
    target_link_libraries(${t} PRIVATE GTest::gtest)
    target_link_libraries(${t} PRIVATE oomph_mpi)
    #target_link_libraries(${t} PRIVATE hwmalloc)
    #target_link_libraries(${t} PRIVATE MPI::MPI_CXX)
    #target_link_libraries(${t} PRIVATE Boost::boost)
    add_test(
        NAME ${t}
        COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${n} ${MPIEXEC_PREFLAGS}
            $<TARGET_FILE:${t}> ${MPIEXEC_POSTFLAGS})
endfunction()

reg_test_mpi(test_mpi_context 4)
