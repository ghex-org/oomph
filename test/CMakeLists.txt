add_subdirectory(mpi_runner)

set(OOMPH_TEST_LEAK_GPU_MEMORY
    OFF
    CACHE BOOL "Do not free memory (bug on Piz Daint)")

# ---------------------------------------------------------------------
# compile tests
# ---------------------------------------------------------------------

# list of serial tests to be executed
set(serial_tests test_unique_function test_unsafe_shared_ptr)

# list of parallel tests to be executed
set(parallel_tests test_context test_send_recv test_send_multi test_cancel
                   test_locality)

# list of parallel tests that also have device code variants
if(HWMALLOC_ENABLE_DEVICE)
  set(device_tests test_send_recv)
endif()

# test_tag_range)
if(OOMPH_ENABLE_BARRIER)
  list(APPEND parallel_tests test_barrier)
endif()

# creates an object library (i.e. *.o file), if DEVICE is specified, extra flags
# are added and the target name has a suffix
function(compile_test t_)
  set(options DEVICE)
  cmake_parse_arguments(CT "${options}" "" "" ${ARGN})
  set(source_filename_ "${t_}.cpp")
  set(suffix_ "")
  if(CT_DEVICE)
    # Make a copy the input source file in the build directory, add a suffix
    set(suffix_ "_device")
    cmake_path(REPLACE_EXTENSION source_filename_ LAST_ONLY "${suffix_}.cpp"
               OUTPUT_VARIABLE src_name_)
    set(dst_file "${CMAKE_CURRENT_BINARY_DIR}/${src_name_}")
    configure_file("${source_filename_}" "${dst_file}" COPYONLY)
    set(source_filename_ "${dst_file}")
  endif()
  set(target_ ${t}${suffix_}_obj)
  add_library(${target_} OBJECT ${source_filename_})
  oomph_target_compile_options(${target_})
  target_compile_definitions(
    ${target_}
    PRIVATE $<$<BOOL:${OOMPH_TEST_LEAK_GPU_MEMORY}>:OOMPH_TEST_LEAK_GPU_MEMORY>)
  target_compile_definitions(
    ${target_} PRIVATE $<$<BOOL:${CT_DEVICE}>:TEST_DEVICE_MODE_ONLY>)
  target_link_libraries(${target_} PRIVATE ext-gtest)
  target_link_libraries(${target_} PUBLIC oomph)
endfunction()

# compile an object library for each test tests will be compiled only once and
# then linked against all enabled oomph backends
list(APPEND all_tests ${serial_tests} ${parallel_tests})
list(REMOVE_DUPLICATES all_tests)
foreach(t ${all_tests})
  compile_test(${t})
  if(${t} IN_LIST device_tests)
    # generate a second version of the obj file, but with DEVICE code enabled
    compile_test(${t} DEVICE)
  endif()
endforeach()

# ---------------------------------------------------------------------
# link and register tests
# ---------------------------------------------------------------------

function(reg_serial_test t)
  add_executable(${t} $<TARGET_OBJECTS:${t}_obj>)
  oomph_target_compile_options(${t})
  target_link_libraries(${t} PRIVATE ext-gtest)
  target_link_libraries(${t} PRIVATE oomph_common)
  add_test(NAME ${t} COMMAND $<TARGET_FILE:${t}>)
endfunction()

foreach(t ${serial_tests})
  reg_serial_test(${t})
endforeach()

# creates an executable by linking to object file and to selected oomph backend
function(reg_parallel_test t_ lib n)
  set(t ${t_}_${lib})
  add_executable(${t} $<TARGET_OBJECTS:${t_}_obj>)
  oomph_target_compile_options(${t})
  target_link_libraries(${t} PRIVATE gtest_main_mpi)
  target_link_libraries(${t} PRIVATE oomph_${lib})
  add_test(NAME ${t}
           COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${n}
                   ${MPIEXEC_PREFLAGS} $<TARGET_FILE:${t}> ${MPIEXEC_POSTFLAGS})
  set_tests_properties(${t} PROPERTIES RUN_SERIAL TRUE)
endfunction()

if(OOMPH_WITH_MPI)
  foreach(t ${parallel_tests})
    reg_parallel_test(${t} mpi 4)
  endforeach()
  foreach(t ${device_tests})
    reg_parallel_test(${t}_device mpi 4)
  endforeach()
endif()

if(OOMPH_WITH_UCX)
  foreach(t ${parallel_tests})
    reg_parallel_test(${t} ucx 4)
  endforeach()
  foreach(t ${device_tests})
    reg_parallel_test(${t}_device ucx 4)
  endforeach()
endif()

if(OOMPH_WITH_LIBFABRIC)
  foreach(t ${parallel_tests})
    reg_parallel_test(${t} libfabric 4)
  endforeach()
  foreach(t ${device_tests})
    reg_parallel_test(${t}_device libfabric 4)
  endforeach()
endif()

add_subdirectory(bindings)
