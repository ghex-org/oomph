# ---------------------------------------------------------------------
# compile tests
# ---------------------------------------------------------------------

# list of tests to be executed
set(parallel_tests_f context_f)

# creates an object library (i.e. *.o file)
function(compile_test_f t_)
    set(t ${t_}_obj)
    add_library(${t} OBJECT test_${t_}.f90)
    oomph_target_compile_options(${t})
    target_link_libraries(${t} PUBLIC MPI::MPI_Fortran)
endfunction()

# compile an object library for each test
# tests will be compiled only once and then linked against all enabled oomph backends
foreach(t ${parallel_tests_f})
    compile_test_f(${t})
endforeach()

# ---------------------------------------------------------------------
# link and register tests
# ---------------------------------------------------------------------

# creates an executable by linking to object file and to selected oomph backend
function(reg_parallel_test_f t_ lib n)
    set(t ${t_}_${lib})
    add_executable(${t} $<TARGET_OBJECTS:${t_}_obj>)
    oomph_target_compile_options(${t})
    target_link_libraries(${t} PRIVATE oomph_f)
    target_link_libraries(${t} PRIVATE oomph_${lib})
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
