set(BUILD-TRANSFER-GRAPH build-transfer-graph)

add_executable(${BUILD-TRANSFER-GRAPH} build-transfer-graph.cpp)
target_compile_definitions(${BUILD-TRANSFER-GRAPH} PUBLIC BINARY_NAME=${BUILD-TRANSFER-GRAPH})
target_include_directories(${BUILD-TRANSFER-GRAPH} PUBLIC ${CMAKE_SOURCE_DIR})
