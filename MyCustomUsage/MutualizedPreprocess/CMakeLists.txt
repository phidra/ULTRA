# this lib depends on :
#   - BoostGeometry (expected to be available in system libs)
#   - libosmium (expected to be available in system libs)
#   - rapidjson (used via conan, see below)

add_subdirectory(GtfsParsing)

add_library(mutualizedpreprocess STATIC
    geojson.cpp
    polygon.cpp
    Graph/extending_with_stops.cpp
    Graph/osmparsing.cpp
    Graph/graph.cpp
    Graph/walking_graph.cpp
    Parsing/polygonfile.cpp
)
include(CMakePrintHelpers)

# to allow that the inclusion is prefixed by "MutualizedPreprocess" (#include "MutualizedPreprocess/geojson.h"), we use parent directory as include dir :
get_filename_component(MUTUALIZEDPREPROCESS_PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
target_include_directories(mutualizedpreprocess PUBLIC "${MUTUALIZEDPREPROCESS_PARENT_DIR}")

# we export the use of conan's rapidjson (to allow users of MutualizedPreprocess to also use rapidjson) :
target_include_directories(mutualizedpreprocess PUBLIC "${CONAN_INCLUDE_DIRS_RAPIDJSON}")
target_link_libraries(mutualizedpreprocess PUBLIC gtfs_parsing)

set(LIBOSMIUM_LINK_DEPS bz2 z expat -pthread)
target_link_libraries(mutualizedpreprocess PRIVATE "${LIBOSMIUM_LINK_DEPS}")
