cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_COMPILER "clang-9")
#set(CMAKE_CXX_STANDARD 17)

set(BIN binary-analyzer)
project($(BIN))
set(CMAKE_BUILD_TYPE Release)

add_executable(${BIN}
    binary-analyzer.cpp
)

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


target_compile_options(${BIN} PRIVATE
    ${COMMON_COMPILE_OPTIONS}
    ${OPTIMIZATION_COMPILE_OPTIONS}
    ${RELEASE_COMPILE_OPTIONS}
)
target_link_libraries(${BIN}
    c++abi
    c++
    m
)
target_include_directories(${BIN} PUBLIC ${CMAKE_SOURCE_DIR})
