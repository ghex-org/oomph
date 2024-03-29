add_executable(sizes sizes.cpp)
target_link_libraries(sizes PRIVATE oomph)

add_library(oomph_f SHARED)
set_target_properties(oomph_f PROPERTIES Fortran_MODULE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Mod)

target_include_directories(oomph_f PRIVATE
    $<INSTALL_INTERFACE:..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../..>)
#if (OOMPH_ENABLE_BARRIER)
#    target_compile_definitions(oomph_f PRIVATE OOMPH_ENABLE_BARRIER)
#endif()

oomph_target_compile_options(oomph_f)
target_link_libraries(oomph_f PRIVATE oomph_common)
target_link_libraries(oomph_f PUBLIC MPI::MPI_Fortran)

if(OOMPH_FORTRAN_OPENMP)
  target_link_libraries(oomph_f PUBLIC OpenMP::OpenMP_Fortran)
  target_link_libraries(oomph_f PUBLIC OpenMP::OpenMP_CXX)
endif()

target_sources(oomph_f PRIVATE object_wrapper.cpp)
target_sources(oomph_f PRIVATE context_bind.cpp)
target_sources(oomph_f PRIVATE communicator_bind.cpp)
target_sources(oomph_f PRIVATE request_bind.cpp)
target_sources(oomph_f PRIVATE message_bind.cpp)
target_sources(oomph_f PRIVATE oomph_sizes.hpp)

target_sources(oomph_f PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/oomph_defs.f90)
target_sources(oomph_f PUBLIC oomph_mod.f90)
target_sources(oomph_f PUBLIC oomph_communicator_mod.f90)
target_sources(oomph_f PUBLIC oomph_message_mod.f90)
target_sources(oomph_f PUBLIC oomph_request_mod.f90)

add_custom_command(OUTPUT oomph_sizes.hpp
  COMMAND "${CMAKE_CURRENT_BINARY_DIR}/sizes" ">oomph_sizes.hpp"
  DEPENDS sizes)

install(TARGETS oomph_f
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/Mod/oomph_defs.mod
        ${CMAKE_CURRENT_BINARY_DIR}/Mod/oomph_mod.mod
        ${CMAKE_CURRENT_BINARY_DIR}/Mod/oomph_communicator_mod.mod
        ${CMAKE_CURRENT_BINARY_DIR}/Mod/oomph_message_mod.mod
        ${CMAKE_CURRENT_BINARY_DIR}/Mod/oomph_request_mod.mod
    DESTINATION
        ${CMAKE_INSTALL_INCLUDEDIR}/oomph/bindings/fortran)

