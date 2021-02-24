set(BUILD-TRANSFER-GRAPH build-transfer-graph)

add_executable(${BUILD-TRANSFER-GRAPH} build-transfer-graph.cpp ${CUSTOM_SOURCES})
target_link_libraries(${BUILD-TRANSFER-GRAPH} ad_cppgtfs)
target_link_libraries(${BUILD-TRANSFER-GRAPH} bz2 z expat -pthread)
target_include_directories(${BUILD-TRANSFER-GRAPH} PUBLIC "${CPPGTFS_INCLUDE_DIR}")
target_include_directories(${BUILD-TRANSFER-GRAPH} PUBLIC ${CMAKE_SOURCE_DIR})
