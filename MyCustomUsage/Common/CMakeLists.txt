add_library(common STATIC geojson.cpp polygon.cpp)
include(CMakePrintHelpers)

# to allow this kind of inclusion (#include "Common/geojson.h"), we use parent directory as include dir :
get_filename_component(COMMON_PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
target_include_directories(common PUBLIC "${COMMON_PARENT_DIR}")

# this lib depends on :
#   - BoostGeometry (expected to be available in system libs)
#   - libosmium (expected to be available in system libs)
#   - rapidjson (used via conan)

# exporting the use of conan's rapidjson (to allow users of Common to also use rapidjson) :
target_include_directories(common PUBLIC "${CONAN_INCLUDE_DIRS_RAPIDJSON}")
