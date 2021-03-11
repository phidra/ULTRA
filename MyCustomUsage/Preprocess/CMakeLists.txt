# this lib depends on :
#   - the cmake target 'ad_cppgtfs' (for the library cppgtfs)
#   - the cmake target 'common'
#   - some ULTRA code (DataStructures / Helpers) which has to be includable
#   - BoostGeometry (expected to be available in system libs)
#   - libosmium (expected to be available in system libs)
#   - rapidjson (used via conan, see below)
#   - fast-cpp-csv-parser (used via conan, see below) (we could probably get rid of this dependency, and use the csv parser from cppgtfs)

set(PREPROCESS_SOURCES
    gtfs_processing.cpp
    Graph/extending_with_stops.cpp
    Graph/osmparsing.cpp
    Graph/graph.cpp
    transfer_graph.cpp
    ultra_gtfs_data.cpp
    Parsing/gtfs_stops.cpp
    Parsing/polygonfile.cpp
)

add_library(preprocess STATIC "${PREPROCESS_SOURCES}")

# to allow that the inclusion is prefixed by "Preprocess" (#include "Preprocess/Graph/osmparsing.h"), we use parent directory as include dir :
get_filename_component(PREPROCESS_PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)

# we also need to be able to include ULTRA code, which is included from one level above :
get_filename_component(ULTRA_INCLUDE_DIR "${PREPROCESS_PARENT_DIR}" DIRECTORY)

target_include_directories(preprocess PUBLIC "${PREPROCESS_PARENT_DIR}")
target_include_directories(preprocess PUBLIC "${ULTRA_INCLUDE_DIR}")
target_include_directories(preprocess PRIVATE "${CONAN_INCLUDE_DIRS_FAST-CPP-CSV-PARSER}")
target_include_directories(preprocess PUBLIC "${CPPGTFS_INCLUDE_DIR}")
target_link_libraries(preprocess PUBLIC common)
target_link_libraries(preprocess PRIVATE ad_cppgtfs)

set(LIBOSMIUM_LINK_DEPS bz2 z expat -pthread)
target_link_libraries(preprocess PRIVATE "${LIBOSMIUM_LINK_DEPS}")
