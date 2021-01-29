cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_COMPILER "clang-9")
#set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror -Wunused-parameter")

# ULTRA code is not robust to all warning flags :
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter -Wno-infinite-recursion -Wno-unused-variable -Wno-sign-compare")

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

include(cppgtfs.cmake)

include(converter.cmake)
include(binary-analyzer.cmake)
include(gtfs-checker.cmake)
include(bordeaux-data.cmake)
include(server.cmake)

add_dependencies(${GTFSCHECKER} gtfs_bordeaux)
add_dependencies(${CONVERTER} gtfs_bordeaux)
