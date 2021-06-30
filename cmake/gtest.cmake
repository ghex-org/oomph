include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    #GIT_TAG        release-1.10.0
    GIT_TAG        master
)
FetchContent_GetProperties(googletest)
if(NOT googletest_POPULATED)
    FetchContent_Populate(googletest)
    add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR} EXCLUDE_FROM_ALL)
    # https://github.com/google/googletest/issues/2429
    add_library(GTest::gtest ALIAS gtest)
endif()

#function(reg_test t)
#    add_executable(${t} ${t}.cpp)
#    target_link_libraries(${t} PRIVATE gtest_main)
#    target_link_libraries(${t} PRIVATE hwmalloc)
#    target_link_libraries(${t} PRIVATE Boost::boost)
#    add_test(NAME ${t} COMMAND $<TARGET_FILE:${t}>)
#endfunction()

