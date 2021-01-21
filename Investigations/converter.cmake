set(CONVERTER ultra-converter)

add_executable(${CONVERTER} converter.cpp)
target_link_libraries(${CONVERTER} ad_cppgtfs)
target_include_directories(${CONVERTER} PUBLIC "${CPPGTFS_PATH}")
target_include_directories(${CONVERTER} PUBLIC ${CMAKE_SOURCE_DIR})
