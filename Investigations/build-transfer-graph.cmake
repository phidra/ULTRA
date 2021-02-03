set(BUILD-TRANSFER-GRAPH build-transfer-graph)

add_executable(${BUILD-TRANSFER-GRAPH} build-transfer-graph.cpp ../Custom/Common/polygon.cpp ../Custom/Parsing/polygonfile.cpp)
target_include_directories(${BUILD-TRANSFER-GRAPH} PUBLIC ${CMAKE_SOURCE_DIR})
