set(BUILD-ULTRA-BINARY-DATA build-ultra-binary-data)

add_executable(${BUILD-ULTRA-BINARY-DATA} build-ultra-binary-data.cpp ${CUSTOM_SOURCES})
target_link_libraries(${BUILD-ULTRA-BINARY-DATA} ad_cppgtfs)
target_link_libraries(${BUILD-ULTRA-BINARY-DATA} bz2 z expat -pthread)
target_include_directories(${BUILD-ULTRA-BINARY-DATA} PUBLIC ${CMAKE_SOURCE_DIR})
target_include_directories(${BUILD-ULTRA-BINARY-DATA} PUBLIC "${CPPGTFS_INCLUDE_DIR}")
