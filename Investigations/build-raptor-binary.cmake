set(BUILD-RAPTOR-BINARY build-raptor-binary)

add_executable(${BUILD-RAPTOR-BINARY} build-raptor-binary.cpp ${CUSTOM_SOURCES})
target_link_libraries(${BUILD-RAPTOR-BINARY} ad_cppgtfs)
target_link_libraries(${BUILD-RAPTOR-BINARY} bz2 z expat -pthread)
target_include_directories(${BUILD-RAPTOR-BINARY} PUBLIC ${CMAKE_SOURCE_DIR})
target_include_directories(${BUILD-RAPTOR-BINARY} PUBLIC "${CPPGTFS_INCLUDE_DIR}")
