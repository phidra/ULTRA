#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

this_script_parent="$(realpath "$(dirname "$0")" )"

# === Preparing build
BUILD_DIR="$this_script_parent/_build"
CMAKE_ROOT_DIR="$this_script_parent/MyCustomUsage"
SCRIPTS_DIR="$this_script_parent/MyCustomUsage/Scripts"
DATA_DIR="$this_script_parent/MyCustomUsage/Data"
echo "BUILD_DIR=$BUILD_DIR"
echo "CMAKE_ROOT_DIR=$CMAKE_ROOT_DIR"

echo "To build from scratch :  rm -rf '$BUILD_DIR'"
# rm -rf "$BUILD_DIR"


# === Preparing WORKDIR :
WORKDIR="${this_script_parent}/WORKDIR_build_ultra_binary_data_IDF"
INPUT_POLYGON_FILE="$WORKDIR/INPUT/idf_polygon.geojson"
INPUT_OSM_FILE="$WORKDIR/INPUT/ile-de-france-latest.osm.pbf"
INPUT_GTFS_DATA="$WORKDIR/INPUT/gtfs"
mkdir -p "$WORKDIR/INPUT"
mkdir -p "$INPUT_GTFS_DATA"


# === Building :
mkdir -p "$BUILD_DIR"
conan install --install-folder="$BUILD_DIR" "$CMAKE_ROOT_DIR" --profile="$CMAKE_ROOT_DIR/conanprofile.txt"
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo  -B"$BUILD_DIR" -H"$CMAKE_ROOT_DIR"
make -j -C "$BUILD_DIR" download_gtfs_idf download_osm_idf build-ultra-binary-data


# === Putting data in WORKDIR :
cp "${DATA_DIR}/idf_polygon.geojson" "$INPUT_POLYGON_FILE"
cp "${this_script_parent}/DOWNLOADED_DATA/osm_idf/ile-de-france-latest.osm.pbf" "$INPUT_OSM_FILE"
cp -R "${this_script_parent}/DOWNLOADED_DATA/gtfs_idf/"* "$INPUT_GTFS_DATA"
echo "Using data from WORKDIR = $WORKDIR"
echo ""


# === Preprocessing GTFS data to use parent stations :
mv "$INPUT_GTFS_DATA/stops.txt" "$INPUT_GTFS_DATA/original_stops.txt"
mv "$INPUT_GTFS_DATA/stop_times.txt" "$INPUT_GTFS_DATA/original_stop_times.txt"
"${SCRIPTS_DIR}/use_parent_stations.py" \
    "$INPUT_GTFS_DATA/original_stops.txt" \
    "$INPUT_GTFS_DATA/original_stop_times.txt" \
    "$INPUT_GTFS_DATA/stops.txt" \
    "$INPUT_GTFS_DATA/stop_times.txt"


# === Preprocessing GTFS data to remove transfers between unknown stops :
mv "$INPUT_GTFS_DATA/transfers.txt" "$INPUT_GTFS_DATA/original_transfers.txt"
"${SCRIPTS_DIR}/remove_invalid_transfers.py" \
    "$INPUT_GTFS_DATA/original_transfers.txt" \
    "$INPUT_GTFS_DATA/stops.txt" \
    "$INPUT_GTFS_DATA/transfers.txt"


# === building ULTRA binary data :
WALKSPEED_KMH=4.7
set -o xtrace
"${BUILD_DIR}/bin/build-ultra-binary-data" \
    "$INPUT_GTFS_DATA" \
    "$INPUT_OSM_FILE" \
    "$INPUT_POLYGON_FILE" \
    "$WALKSPEED_KMH" \
    "$WORKDIR"
