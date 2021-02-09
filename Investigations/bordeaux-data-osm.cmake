include(ExternalProject)

set(BORDEAUX_OSM_DESTINATION_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../DOWNLOADED_DATA/osm_bordeaux")

message(STATUS "Bordeaux OSM download URL = '${BORDEAUX_OSM_DESTINATION_DIRECTORY}'")

# NOTE : it seems that if the target directory exists, the external project is not downloaded again

ExternalProject_Add(
    osm_bordeaux
    PREFIX osm_bordeaux
    URL "https://download.geofabrik.de/europe/france/aquitaine-latest.osm.pbf"
    DOWNLOAD_NO_EXTRACT TRUE
    DOWNLOAD_DIR "${BORDEAUX_OSM_DESTINATION_DIRECTORY}"
    SOURCE_DIR "${BORDEAUX_OSM_DESTINATION_DIRECTORY}"
    # we only want to download the archive, thus we disable building :
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    )

