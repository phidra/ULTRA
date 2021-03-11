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
include(CMakePrintHelpers)
cmake_print_variables(CPPGTFS_INCLUDE_DIR)
target_include_directories(preprocess PRIVATE "${CONAN_INCLUDE_DIRS_FAST-CPP-CSV-PARSER}")
target_include_directories(preprocess PUBLIC "${CPPGTFS_INCLUDE_DIR}")
target_include_directories(preprocess PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(preprocess PUBLIC common)

# this lib depends on :

# TODO :

# cppgtfs
# ULTRA DataStructures
# ULTRA Helpers
# Common
# BoostGeometry
# libosmium
# rapidjson
# fast-cpp-csv-parser (header-only), (d'ailleurs, je pourrai peut-être supprimer cette dépendance, et utiliser le parser CSV de cppgtfs ?)
