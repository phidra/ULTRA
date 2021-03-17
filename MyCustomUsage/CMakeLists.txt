cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror -Wunused-parameter")
set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")

project(my-custom-ultra-usage)

# ULTRA code is not robust to all warning flags :
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter -Wno-infinite-recursion -Wno-unused-variable -Wno-sign-compare")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


set(CMAKE_BUILD_TYPE RelWithDebInfo)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

include(cppgtfs.cmake)

add_subdirectory(Common)
add_subdirectory(Preprocess)
add_subdirectory(Server)
add_subdirectory(Mains)

# NOTE : if needed, also toggle compilation flags used by ULTRA
# cf. https://github.com/phidra/ULTRA/blob/eed8bb61dc15f2ab306a5ecfd8c24f2b5562e017/Runnables/Makefile#L5

include(download-bordeaux-data-gtfs.cmake)
include(download-bordeaux-data-osm.cmake)
include(download-idf-data-gtfs.cmake)
include(download-idf-data-osm.cmake)
