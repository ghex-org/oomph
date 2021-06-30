
function(reg_test_ucx t n)
    add_executable(${t} ${t}.cpp)
    target_link_libraries(${t} PRIVATE gtest_main_mpi)
    target_link_libraries(${t} PRIVATE GTest::gtest)
    target_link_libraries(${t} PRIVATE oomph_ucx)
    add_test(
        NAME ${t}
        COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${n} ${MPIEXEC_PREFLAGS}
            $<TARGET_FILE:${t}> ${MPIEXEC_POSTFLAGS})
endfunction()

reg_test_ucx(test_ucx_context 1)

