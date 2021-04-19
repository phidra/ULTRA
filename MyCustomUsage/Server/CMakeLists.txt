# this lib depends on :
#   - some ULTRA code (Algorithms / DataStructures / Helpers) which has to be includable
#   - rapidjson (used via conan, see below)
#   - cpp-httplib (used via conan, see below)

set(SERVER_SOURCES
    json_helper.cpp
    Snapping/snapping.cpp
    Handlers/echo_handler.cpp
    Handlers/journey_handler.cpp
)

add_library(serverlib STATIC "${SERVER_SOURCES}")

# to allow this kind of inclusion (#include "Server/json_helper.h"), we use parent directory as include dir :
get_filename_component(SERVER_PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)

# we also need to be able to include ULTRA code, which is included from one level above :
get_filename_component(ULTRA_INCLUDE_DIR "${SERVER_PARENT_DIR}" DIRECTORY)

target_include_directories(serverlib PUBLIC "${SERVER_PARENT_DIR}")
target_include_directories(serverlib PUBLIC "${ULTRA_INCLUDE_DIR}")
target_include_directories(serverlib PUBLIC "${CONAN_INCLUDE_DIRS_CPP-HTTPLIB}")
target_include_directories(serverlib PRIVATE "${CONAN_INCLUDE_DIRS_RAPIDJSON}")

set(CPPHTTPLIB_LINK_DEPS -pthread)
target_link_libraries(serverlib PUBLIC "${CPPHTTPLIB_LINK_DEPS}")
