include(ExternalProject)

set(BORDEAUX_GTFS_DESTINATION_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../DATA/gtfs_bordeaux")

message(STATUS "Bordeaux GTFS download URL = '${BORDEAUX_GTFS_DESTINATION_DIRECTORY}'")

# NOTE : it seems that if the target directory exists, the external project is not downloaded again

ExternalProject_Add(
    gtfs_bordeaux
    PREFIX gtfs_bordeaux
    URL "https://www.data.gouv.fr/fr/datasets/r/13e7e219-b037-4d60-a3ab-e55d2d3e5291"
    DOWNLOAD_NAME "gtfs_bordeaux.zip"
    URL_HASH MD5="3695f7193376295f331027f1c6b00f6e"
    DOWNLOAD_DIR "${BORDEAUX_GTFS_DESTINATION_DIRECTORY}"
    SOURCE_DIR "${BORDEAUX_GTFS_DESTINATION_DIRECTORY}"
    # we only want to download the archive, thus we disable building :
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    )

