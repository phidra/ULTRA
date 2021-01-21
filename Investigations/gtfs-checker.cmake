set(GTFSCHECKER gtfs-checker)

add_executable(${GTFSCHECKER} gtfs-checker.cpp)
target_link_libraries(${GTFSCHECKER} ad_cppgtfs)
target_include_directories(${GTFSCHECKER} PUBLIC "${CPPGTFS_PATH}")
target_include_directories(${GTFSCHECKER} PUBLIC ${CMAKE_SOURCE_DIR})
