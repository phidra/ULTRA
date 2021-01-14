cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_COMPILER "clang-9")
#set(CMAKE_CXX_STANDARD 17)

set(ANALYZER ultra-binary-analyzer)
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


# analyzer :
add_executable(${ANALYZER} binary-analyzer.cpp)
target_compile_options(${ANALYZER} PRIVATE
    ${COMMON_COMPILE_OPTIONS}
    ${OPTIMIZATION_COMPILE_OPTIONS}
    ${RELEASE_COMPILE_OPTIONS}
)
target_link_libraries(${ANALYZER}
    c++abi
    c++
    m
)
target_include_directories(${ANALYZER} PUBLIC ${CMAKE_SOURCE_DIR})


# server :
add_executable(${SERVER} server.cpp)
target_compile_options(${SERVER} PRIVATE
    ${COMMON_COMPILE_OPTIONS}
    ${OPTIMIZATION_COMPILE_OPTIONS}
    ${RELEASE_COMPILE_OPTIONS}
)
target_link_libraries(${SERVER}
    c++abi
    c++
    m
)
target_include_directories(${SERVER} PUBLIC ${CMAKE_SOURCE_DIR})
