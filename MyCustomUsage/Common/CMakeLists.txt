include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

add_library(common STATIC geojson.cpp polygon.cpp)
target_include_directories(common PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# this lib depends on :
#   - BoostGeometry (expected to be available in system libs)
#   - libosmium (expected to be available in system libs)
#   - rapidjson (used via conan)

# exporting the use of conan's rapidjson (to allow users of Common to also use rapidjson) :
target_include_directories(common PUBLIC "${CONAN_INCLUDE_DIRS_RAPIDJSON}")
