
set(benchmarks bench_1 bench_p2p_bi_ft_wait_mt_1)

set(OOMPH_BENCHMARKS_MT OFF CACHE BOOL "Multithreaded benchmarks")
if (OOMPH_BENCHMARKS_MT)
    find_package(OpenMP REQUIRED)
endif()

# compiler flags for benchmarks
function(oomph_benchmark_compile_options b)
    oomph_target_compile_options(${b})
    if (OOMPH_BENCHMARKS_MT)
        target_compile_definitions(${b} PRIVATE OOMPH_BENCHMARKS_MT)
    endif()
endfunction()

# creates an object library (i.e. *.o file)
function(compile_benchmark t_)
    set(t ${t_}_obj)
    add_library(${t} OBJECT ${t_}.cpp)
    oomph_benchmark_compile_options(${t})
    target_link_libraries(${t} PRIVATE MPI::MPI_CXX)
    target_link_libraries(${t} PUBLIC oomph)
    if (OOMPH_BENCHMARKS_MT)
        target_link_libraries(${t} PRIVATE OpenMP::OpenMP_CXX)
    endif()
endfunction()

# compile an object library for each benchmark
foreach(t ${benchmarks})
    compile_benchmark(${t})
endforeach()

# creates an executable by linking to object file and to selected oomph backend
function(make_benchmark t_ lib)
    set(t ${t_}_${lib})
    add_executable(${t} $<TARGET_OBJECTS:${t_}_obj>)
    oomph_benchmark_compile_options(${t})
    target_link_libraries(${t} PRIVATE oomph_${lib})
    if (OOMPH_BENCHMARKS_MT)
        target_link_libraries(${t} PRIVATE OpenMP::OpenMP_CXX)
    endif()
endfunction()

if (OOMPH_WITH_MPI)
    foreach(t ${benchmarks})
        make_benchmark(${t} mpi)
    endforeach()
endif()

if (OOMPH_WITH_UCX)
    foreach(t ${benchmarks})
        make_benchmark(${t} ucx)
    endforeach()
endif()

if (OOMPH_WITH_LIBFABRIC)
    foreach(t ${benchmarks})
        make_benchmark(${t} libfabric)
    endforeach()
endif()
