# this lib depends on :
#   - the cmake target 'graphlib'
#   - the cmake target 'gtfs_parsing'
#   - some ULTRA code (DataStructures / Helpers) which has to be includable


set(PREPROCESS_SOURCES
    ultra_transfer_data.cpp
    ultra_gtfs_data.cpp
)

add_library(preprocess STATIC "${PREPROCESS_SOURCES}")

# to allow that the inclusion is prefixed by "Preprocess" (#include "Preprocess/Graph/osmparsing.h"), we use parent directory as include dir :
get_filename_component(PREPROCESS_PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)

# we also need to be able to include ULTRA code, which is included from one level above :
get_filename_component(ULTRA_INCLUDE_DIR "${PREPROCESS_PARENT_DIR}" DIRECTORY)

target_include_directories(preprocess PUBLIC "${PREPROCESS_PARENT_DIR}")
target_include_directories(preprocess PUBLIC "${ULTRA_INCLUDE_DIR}")
target_link_libraries(preprocess PUBLIC graphlib)
target_link_libraries(preprocess PUBLIC gtfs_parsing)
