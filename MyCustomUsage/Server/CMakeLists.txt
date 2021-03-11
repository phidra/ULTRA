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

target_link_libraries(serverlib PUBLIC common)
target_link_libraries(serverlib PUBLIC -pthread)  # needed by cpp-httplib

# this lib depends on :

# rapidjson (but included in Common anyway)
# common
# NOTE : la dépendance à Common ne concerne que duration_helper...
#        ... et celui-ci n'est de toutes façons nécessaire QUE pour serverlib
#        je pourrai donc n'avoir que deux modules de code indépendants : Preprocess et Server
