cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror -Wunused-parameter")
set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")

# ULTRA code is not robust to all warning flags :
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter -Wno-infinite-recursion -Wno-unused-variable -Wno-sign-compare")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


project(ultra-comprehension)
set(CMAKE_BUILD_TYPE RelWithDebInfo)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()


set(COMMON_COMPILE_OPTIONS
    -fopenmp
    -pipe
)
set(RELEASE_COMPILE_OPTIONS
    -ffast-math
    -ftree-vectorize
    -Wfatal-errors
)
set(OPTIMIZATION_COMPILE_OPTIONS
    -march=native
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

file(GLOB_RECURSE CUSTOM_SOURCES RELATIVE "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/../Custom/*.cpp")

include(cppgtfs.cmake)

include(converter.cmake)
include(binary-analyzer.cmake)
include(gtfs-checker.cmake)
include(bordeaux-data-gtfs.cmake)
include(bordeaux-data-osm.cmake)
include(server.cmake)
include(build-raptor-binary.cmake)

add_dependencies(${GTFSCHECKER} gtfs_bordeaux)
add_dependencies(${CONVERTER} gtfs_bordeaux)
add_dependencies(${BUILD-RAPTOR-BINARY} osm_bordeaux gtfs_bordeaux)
