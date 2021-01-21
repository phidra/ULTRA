cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_COMPILER "clang-9")
#set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror -Wunused-parameter")

# ULTRA code is not robust to all warning flags :
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter -Wno-infinite-recursion -Wno-unused-variable -Wno-sign-compare")

set(ANALYZER ultra-binary-analyzer)
set(CONVERTER ultra-converter)
set(GTFSCHECKER gtfs-checker)
set(SERVER ultra-server)
project(ultra-comprehension)
set(CMAKE_BUILD_TYPE Release)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()


set(COMMON_COMPILE_OPTIONS
    -std=c++17
    -fopenmp
    -pipe
    -stdlib=libc++
)
set(RELEASE_COMPILE_OPTIONS
    -ffast-math
    -ftree-vectorize
    -Wfatal-errors
    -DNDEBUG
)
set(OPTIMIZATION_COMPILE_OPTIONS
    -march=native
    -O3
)
set(DEBUG_COMPILE_OPTIONS
    -rdynamic
    -Wpedantic
    -pedantic-errors
    -Wparentheses
    -Wfatal-errors
    -D_GLIBCXX_DEBUG
    -g
    -fno-omit-frame-pointer
)
add_compile_options(
    ${COMMON_COMPILE_OPTIONS}
    ${OPTIMIZATION_COMPILE_OPTIONS}
    ${RELEASE_COMPILE_OPTIONS}
)

set(LIBS
    c++abi
    c++
    m
)
link_libraries(${LIBS})

set (CPPGTFS_PATH "../libs/cppgtfs")
add_subdirectory ("${CPPGTFS_PATH}" "${CMAKE_BINARY_DIR}/cppgtfs")


# analyzer :
add_executable(${ANALYZER} binary-analyzer.cpp)
target_include_directories(${ANALYZER} PUBLIC ${CMAKE_SOURCE_DIR})


# converter :
add_executable(${CONVERTER} converter.cpp)
target_link_libraries(${CONVERTER} ad_cppgtfs)
target_include_directories(${CONVERTER} PUBLIC "${CPPGTFS_PATH}")
target_include_directories(${CONVERTER} PUBLIC ${CMAKE_SOURCE_DIR})

# gtfs-checker :
add_executable(${GTFSCHECKER} gtfs-checker.cpp)
target_link_libraries(${GTFSCHECKER} ad_cppgtfs)
target_include_directories(${GTFSCHECKER} PUBLIC "${CPPGTFS_PATH}")
target_include_directories(${GTFSCHECKER} PUBLIC ${CMAKE_SOURCE_DIR})


# server :
add_executable(${SERVER} server.cpp)
target_include_directories(${SERVER} PUBLIC ${CMAKE_SOURCE_DIR})


# gtfs Bordeaux :
include(ExternalProject)

set(BORDEAUX_GTFS_DOWNLOAD_URL "https://www.data.gouv.fr/fr/datasets/r/13e7e219-b037-4d60-a3ab-e55d2d3e5291")
set(BORDEAUX_GTFS_ARCHIVE_MD5 "e41f692a684aba32e652dc03b15615f9")
set(BORDEAUX_GTFS_DESTINATION_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../DATA/gtfs_bordeaux")

message(STATUS "Bordeaux GTFS download URL = '${BORDEAUX_GTFS_DESTINATION_DIRECTORY}'")

# NOTE : it seems that if the target directory exists, the external project is not downloaded again

ExternalProject_Add(
    gtfs_bordeaux
    PREFIX gtfs_bordeaux
    URL "${BORDEAUX_GTFS_DOWNLOAD_URL}"
    DOWNLOAD_NAME "gtfs_bordeaux.zip"
    URL_HASH MD5=${BORDEAUX_GTFS_ARCHIVE_MD5}
    DOWNLOAD_DIR "${BORDEAUX_GTFS_DESTINATION_DIRECTORY}"
    SOURCE_DIR "${BORDEAUX_GTFS_DESTINATION_DIRECTORY}"
    # we only want to download the archive, thus we disable building :
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    )

add_dependencies(${GTFSCHECKER} gtfs_bordeaux)
