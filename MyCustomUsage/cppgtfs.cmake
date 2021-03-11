include(FetchContent)

FetchContent_Declare(
    cppgtfs
    GIT_REPOSITORY "https://ad-git.informatik.uni-freiburg.de/ad/cppgtfs"
    GIT_TAG "master"
)


# Check if population has already been performed
FetchContent_GetProperties(cppgtfs)
if(NOT cppgtfs_POPULATED)
    # Fetch the content using previously declared details
    FetchContent_Populate(cppgtfs)

    # until I submit a PR with a fix, cppgtfs needs to be patched to rename pow10 :
    execute_process(COMMAND sed -i "s@pow10@powersOf10@g" "${cppgtfs_SOURCE_DIR}/src/ad/util/CsvParser.cpp")

    # Bring the populated content into the build
    add_subdirectory(${cppgtfs_SOURCE_DIR} ${cppgtfs_BINARY_DIR})
endif()

set (CPPGTFS_INCLUDE_DIR "${cppgtfs_SOURCE_DIR}/src")
