#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

this_script_parent="$(realpath "$(dirname "$0")" )"

BUILD_DIR="$this_script_parent/_build"
CMAKE_ROOT_DIR="$this_script_parent/Investigations"
echo "BUILD_DIR=$BUILD_DIR"
echo "CMAKE_ROOT_DIR=$CMAKE_ROOT_DIR"

echo "To build from scratch :  rm -rf '$BUILD_DIR'"
# rm -rf "$BUILD_DIR"

pushd "$CMAKE_ROOT_DIR"
mkdir -p "$BUILD_DIR"
conan install --install-folder="$BUILD_DIR" . --profile="conanprofile.txt"
cmake -B"$BUILD_DIR" -H"$CMAKE_ROOT_DIR"
make -j -C "$BUILD_DIR" osm_idf gtfs_idf
make -j -C "$BUILD_DIR" build-ultra-binary-data
popd

WORKDIR="${this_script_parent}/WORKDIR_build_ultra_binary_data_IDF"
mkdir -p "$WORKDIR/INPUT"
POLYGON_FILE="$WORKDIR/INPUT/idf_polygon.geojson"
OSM_FILE="$WORKDIR/INPUT/ile-de-france-latest.osm.pbf"
GTFS_DATA="$WORKDIR/INPUT/gtfs"
mkdir -p "$GTFS_DATA"

cp "${this_script_parent}/data/idf_polygon.geojson" "$POLYGON_FILE"
cp "${this_script_parent}/DOWNLOADED_DATA/osm_idf/ile-de-france-latest.osm.pbf" "$OSM_FILE"
cp -R "${this_script_parent}/DOWNLOADED_DATA/gtfs_idf/"* "$GTFS_DATA"
echo "Using data from WORKDIR = $WORKDIR"
echo ""

# preprocessing GTFS data to use parent stations :
mv "$GTFS_DATA/stops.txt" "$GTFS_DATA/original_stops.txt"
mv "$GTFS_DATA/stop_times.txt" "$GTFS_DATA/original_stop_times.txt"
"${this_script_parent}/Scripts/use_parent_stations.py" \
    "$GTFS_DATA/original_stops.txt" \
    "$GTFS_DATA/original_stop_times.txt" \
    "$GTFS_DATA/stops.txt" \
    "$GTFS_DATA/stop_times.txt"

# preprocessing GTFS data to remove transfers between unknown stops :
mv "$GTFS_DATA/transfers.txt" "$GTFS_DATA/original_transfers.txt"
"${this_script_parent}/Scripts/remove_invalid_transfers.py" \
    "$GTFS_DATA/original_transfers.txt" \
    "$GTFS_DATA/stops.txt" \
    "$GTFS_DATA/transfers.txt"

set -o xtrace
"${BUILD_DIR}/bin/build-ultra-binary-data" \
    "$GTFS_DATA" \
    "$OSM_FILE" \
    "$POLYGON_FILE" \
    "$WORKDIR"
