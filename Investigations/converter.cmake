set(CONVERTER ultra-converter)

add_executable(${CONVERTER} converter.cpp ${CUSTOM_SOURCES})
target_link_libraries(${CONVERTER} ad_cppgtfs)
target_link_libraries(${CONVERTER} bz2 z expat -pthread)
target_include_directories(${CONVERTER} PUBLIC "${CPPGTFS_INCLUDE_DIR}")
target_include_directories(${CONVERTER} PUBLIC ${CMAKE_SOURCE_DIR})
