if(NOT _hwmalloc_already_fetched)
    find_package(HWMALLOC QUIET)
endif()
if(NOT HWMALLOC_FOUND)
    set(_hwmalloc_repository "https://github.com/boeschf/hwmalloc.git")
    #set(_hwmalloc_tag        "master")
    set(_hwmalloc_tag        "oomph_integration")
    if(NOT _hwmalloc_already_fetched)
        message(STATUS "Fetching HWMALLOC ${_hwmalloc_tag} from ${_hwmalloc_repository}")
    endif()
    include(FetchContent)
    FetchContent_Declare(
        hwmalloc
        GIT_REPOSITORY ${_hwmalloc_repository}
        GIT_TAG        ${_hwmalloc_tag}
    )
    FetchContent_MakeAvailable(hwmalloc)
    set(_hwmalloc_already_fetched ON CACHE INTERNAL "")
endif()


