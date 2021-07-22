include(gtest)

add_subdirectory(mpi_runner)

set(_tests test_context test_send_recv test_send_multi test_barrier)

# creates an object library
function(compile_test t_)
    set(t ${t_}_obj)
    add_library(${t} OBJECT ${t_}.cpp)
    target_link_libraries(${t} PRIVATE GTest::gtest)
    target_link_libraries(${t} PUBLIC oomph_mpi)
endfunction()

# compile an object library for each test
# tests will be compiled only once and then linked against all enabled oomph backends
foreach(t ${_tests})
    compile_test(${t})
endforeach()

# creates an executable by linking to object file and to selected oomph backend
# registers test with ctest, suffix indicates oomph backend
function(reg_test t_ lib n)
    set(t ${t_}_${lib})
    add_executable(${t} $<TARGET_OBJECTS:${t_}_obj>)
    target_link_libraries(${t} PRIVATE gtest_main_mpi)
    target_link_libraries(${t} PRIVATE oomph_${lib})
    add_test(
        NAME ${t}
        COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${n} ${MPIEXEC_PREFLAGS}
            $<TARGET_FILE:${t}> ${MPIEXEC_POSTFLAGS})
endfunction()

if (OOMPH_WITH_MPI)
    foreach(t ${_tests})
        reg_test(${t} mpi 4)
    endforeach()
endif()

if (OOMPH_WITH_UCX)
    reg_test(test_context ucx 4)
endif()

if (OOMPH_WITH_LIBFABRIC)
    foreach(t ${_tests})
        reg_test(${t} libfabric 4)
    endforeach()
endif()
