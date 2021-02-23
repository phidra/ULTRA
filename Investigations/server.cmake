set(SERVER ultra-server)

add_executable(${SERVER} server.cpp ${CUSTOM_SOURCES})
target_link_libraries(${SERVER} bz2 z expat -pthread)
target_include_directories(${SERVER} PUBLIC ${CMAKE_SOURCE_DIR})
