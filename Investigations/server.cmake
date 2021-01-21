set(SERVER ultra-server)

add_executable(${SERVER} server.cpp)
target_include_directories(${SERVER} PUBLIC ${CMAKE_SOURCE_DIR})
