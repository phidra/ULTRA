set(BUILD-TRANSFER-GRAPH build-transfer-graph)

add_executable(${BUILD-TRANSFER-GRAPH} build-transfer-graph.cpp ../Custom/Common/polygon.cpp ../Custom/Parsing/polygonfile.cpp ../Custom/Graph/graph.cpp ../Custom/Graph/osmparsing.cpp ../Custom/Graph/extending_with_stops.cpp)
target_link_libraries(${BUILD-TRANSFER-GRAPH} bz2 z expat -pthread)
target_include_directories(${BUILD-TRANSFER-GRAPH} PUBLIC ${CMAKE_SOURCE_DIR})
