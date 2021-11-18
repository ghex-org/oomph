set(OOMPH_BUILD_FORTRAN OFF CACHE BOOL "build FORTRAN bindings")

if (OOMPH_BUILD_FORTRAN)
    enable_language(Fortran)
    set(OOMPH_FORTRAN_FP "float" CACHE STRING "Floating-point type")
    set_property(CACHE OOMPH_FORTRAN_FP PROPERTY STRINGS "float" "double")
    if(${OOMPH_FORTRAN_FP} STREQUAL "float")
        set(OOMPH_FORTRAN_FP_KIND 4)
    else()
        set(OOMPH_FORTRAN_FP_KIND 8)
    endif()

    #set(OOMPH_FORTRAN_OPENMP "ON" CACHE BOOL "Compile Fortran bindings with OpenMP")

    find_package(MPI REQUIRED)

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/oomph_defs.f90.in
        ${CMAKE_CURRENT_BINARY_DIR}/bindings/fortran/oomph_defs.f90 @ONLY)
    install(FILES ${PROJECT_BINARY_DIR}/bindings/fortran/oomph_defs.f90
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/oomph/bindings/fortran)

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/oomph_defs.hpp.in
        ${CMAKE_CURRENT_BINARY_DIR}/bindings/fortran/oomph_defs.hpp @ONLY)
    #install(FILES ${PROJECT_BINARY_DIR}/bindings/fortran/oomph_defs.hpp
    #    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/bindings/fortran)
endif()
