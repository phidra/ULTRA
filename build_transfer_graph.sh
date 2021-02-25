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
make -j -C "$BUILD_DIR" build-transfer-graph
popd

WORKDIR="${this_script_parent}/WORKDIR_build_transfer_graph"
mkdir -p "$WORKDIR/INPUT"
POLYGON_FILE="$WORKDIR/INPUT/bordeaux_polygon.geojson"
OSM_FILE="$WORKDIR/INPUT/aquitaine-latest.osm.pbf"
STOPS_FILE="$WORKDIR/INPUT/bordeaux_stops.txt"

# cp "${this_script_parent}/data/bordeaux_polygon_TEST.geojson" "$POLYGON_FILE"
cp "${this_script_parent}/data/bordeaux_polygon.geojson" "$POLYGON_FILE"
cp "${this_script_parent}/DOWNLOADED_DATA/osm_bordeaux/aquitaine-latest.osm.pbf" "$OSM_FILE"
cp "${this_script_parent}/DOWNLOADED_DATA/gtfs_bordeaux/stops.txt" "$STOPS_FILE"
echo "Using data from WORKDIR = $WORKDIR"
echo ""

set -o xtrace
"${BUILD_DIR}/bin/build-transfer-graph" \
    "$OSM_FILE" \
    "$POLYGON_FILE" \
    "$STOPS_FILE" \
    "$WORKDIR"