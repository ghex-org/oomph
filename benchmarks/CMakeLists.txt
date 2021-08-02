
set(benchmarks
    bench_p2p_bi_ft_wait
    bench_p2p_bi_ft_avail
    bench_p2p_bi_cb_wait
    bench_p2p_bi_cb_avail)

set(OOMPH_BENCHMARKS_MT OFF CACHE BOOL "Multithreaded benchmarks")
if (OOMPH_BENCHMARKS_MT)
    find_package(OpenMP REQUIRED)
endif()

function(compile_benchmark_impl t_ t)
    add_library(${t} OBJECT ${t_}_mt.cpp)
    oomph_target_compile_options(${t})
    target_link_libraries(${t} PRIVATE MPI::MPI_CXX)
    target_link_libraries(${t} PUBLIC oomph)
endfunction()

function(compile_benchmark t_)
    set(t ${t_}_obj)
    compile_benchmark_impl(${t_} ${t})
endfunction()

function(compile_benchmark_mt t_)
    set(t ${t_}_mt_obj)
    compile_benchmark_impl(${t_} ${t})
    target_compile_definitions(${t} PRIVATE OOMPH_BENCHMARKS_MT)
    target_link_libraries(${t} PRIVATE OpenMP::OpenMP_CXX)
endfunction()

function(make_benchmark_impl t_ t lib)
    add_executable(${t} $<TARGET_OBJECTS:${t_}_obj>)
    oomph_target_compile_options(${t})
    target_link_libraries(${t} PRIVATE oomph_${lib})
endfunction()

function(make_benchmark t_ lib)
    set(t ${t_}_${lib})
    make_benchmark_impl(${t_} ${t} ${lib})
endfunction()

function(make_benchmark_mt t_ lib)
    set(t ${t_}_mt_${lib})
    make_benchmark_impl(${t_} ${t} ${lib})
    target_compile_definitions(${t} PRIVATE OOMPH_BENCHMARKS_MT)
    target_link_libraries(${t} PRIVATE OpenMP::OpenMP_CXX)
endfunction()

# compile an object library for each benchmark
foreach(t ${benchmarks})
    compile_benchmark(${t})
    if (OOMPH_BENCHMARKS_MT)
        compile_benchmark_mt(${t})
    endif()
endforeach()

# link to object libraries with each backend
if (OOMPH_WITH_MPI)
    foreach(t ${benchmarks})
        make_benchmark(${t} mpi)
        if (OOMPH_BENCHMARKS_MT)
            make_benchmark_mt(${t} mpi)
        endif()
    endforeach()
endif()

if (OOMPH_WITH_UCX)
    foreach(t ${benchmarks})
        make_benchmark(${t} ucx)
        if (OOMPH_BENCHMARKS_MT)
            make_benchmark_mt(${t} ucx)
        endif()
    endforeach()
endif()

if (OOMPH_WITH_LIBFABRIC)
    foreach(t ${benchmarks})
        make_benchmark(${t} libfabric)
        if (OOMPH_BENCHMARKS_MT)
            make_benchmark_mt(${t} libfabric)
        endif()
    endforeach()
endif()
