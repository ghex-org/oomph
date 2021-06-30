if(NOT _hwmalloc_already_fetched)
    find_package(Hwmalloc QUIET)
endif()
if(NOT Hwmalloc_FOUND)
    #set(_hwmalloc_repository "https://github.com/boeschf/hwmalloc")
    set(_hwmalloc_repository "git@github.com:boeschf/hwmalloc.git")
    set(_hwmalloc_tag        "ucx_support")
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


