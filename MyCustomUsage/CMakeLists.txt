cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror -Wunused-parameter")

project(ultra)

# ULTRA code is not robust to all warning flags :
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter -Wno-infinite-recursion -Wno-unused-variable -Wno-sign-compare")

# uncomment (and adapt if necessary) to use iwyu (https://include-what-you-use.org/) :
# note : there is currently a slight bug in iwyu, that causes unnecessary long paths, such as :
#       #include "DataStructures/RAPTOR/Entities/../../../Helpers/IO/Serialization.h"
# see https://github.com/include-what-you-use/include-what-you-use/issues/532
# the benefits seem greater than this bug, though, so iwyu is kept
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/usr/lib/llvm-10/lib/clang/10.0.0/include ")
# set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "/usr/bin/iwyu;-Xiwyu;--no_comments;-Xiwyu;--cxx17ns")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)


add_subdirectory(Preprocess)
add_subdirectory(Server)
add_subdirectory(Mains)

# NOTE : if needed, also toggle compilation flags used by ULTRA
# cf. https://github.com/phidra/ULTRA/blob/eed8bb61dc15f2ab306a5ecfd8c24f2b5562e017/Runnables/Makefile#L5


# we use FetchContent to manage dependency to upstream repo :
# https://cmake.org/cmake/help/latest/module/FetchContent.html#command:fetchcontent_makeavailable
include(FetchContent)
FetchContent_Declare(
    unrestricted-walking-preprocess
    GIT_REPOSITORY "https://github.com/phidra/unrestricted-walking-preprocess"
    GIT_TAG "master"
) 
FetchContent_MakeAvailable(unrestricted-walking-preprocess)
message(STATUS "        unrestricted-walking-preprocess_SOURCE_DIR = ${unrestricted-walking-preprocess_SOURCE_DIR}")
message(STATUS "        unrestricted-walking-preprocess_BINARY_DIR = ${unrestricted-walking-preprocess_BINARY_DIR}")
message(STATUS "        unrestricted-walking-preprocess_POPULATED  = ${unrestricted-walking-preprocess_POPULATED}")

# upstream repo unrestricted-walking-preprocessing needs RAPIDJSON_INCLUDE_DIR to find rapidjson :
set(RAPIDJSON_INCLUDE_DIR "${CONAN_INCLUDE_DIRS_RAPIDJSON}")

add_subdirectory("${unrestricted-walking-preprocess_SOURCE_DIR}/src" ${unrestricted-walking-preprocess_BINARY_DIR})
