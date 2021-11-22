# list of tests to be executed
set(parallel_tests_f context_f)

function(reg_parallel_test_f t_ lib n)
    set(t ${t_}_${lib})
    add_executable(${t} test_${t_}.f90)
    oomph_target_compile_options(${t})
    target_link_libraries(${t} PRIVATE oomph_f)
    target_link_libraries(${t} PRIVATE oomph_${lib})
    target_link_libraries(${t} PUBLIC MPI::MPI_Fortran)
    add_test(
        NAME ${t}
        COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${n} ${MPIEXEC_PREFLAGS}
            $<TARGET_FILE:${t}> ${MPIEXEC_POSTFLAGS})
endfunction()

if (OOMPH_WITH_MPI)
    foreach(t ${parallel_tests_f})
        reg_parallel_test_f(${t} mpi 4)
    endforeach()
endif()

if (OOMPH_WITH_UCX)
    foreach(t ${parallel_tests_f})
        reg_parallel_test_f(${t} ucx 4)
    endforeach()
endif()

if (OOMPH_WITH_LIBFABRIC)
    foreach(t ${parallel_tests_f})
        reg_parallel_test_f(${t} libfabric 4)
    endforeach()
endif()
