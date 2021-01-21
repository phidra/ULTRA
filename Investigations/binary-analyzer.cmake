set(ANALYZER ultra-binary-analyzer)

add_executable(${ANALYZER} binary-analyzer.cpp)
target_include_directories(${ANALYZER} PUBLIC ${CMAKE_SOURCE_DIR})
