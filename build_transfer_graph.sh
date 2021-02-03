#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

this_script_parent="$(realpath "$(dirname "$0")" )"

BUILD_DIR="$this_script_parent/_build/build-transfer-graph"
CMAKE_ROOT_DIR="$this_script_parent/Investigations"
echo "BUILD_DIR=$BUILD_DIR"
echo "CMAKE_ROOT_DIR=$CMAKE_ROOT_DIR"

echo "To build from scratch :  rm -rf '$BUILD_DIR'"
# rm -rf "$BUILD_DIR"

pushd "$CMAKE_ROOT_DIR"
mkdir -p "$BUILD_DIR"
conan install --install-folder="$BUILD_DIR" .
cmake -B"$BUILD_DIR" -H"$CMAKE_ROOT_DIR"
make -j -C "$BUILD_DIR" build-transfer-graph
popd

# run server :
WORKDIR="${this_script_parent}/WORKDIR_build_transfer_graph"
mkdir -p "$WORKDIR"
POLYGON_FILE="$WORKDIR/bordeaux_polygon.geojson"
cp "${this_script_parent}/data/bordeaux_polygon.geojson" "$POLYGON_FILE"
echo "Using data from WORKDIR = $WORKDIR"
"${BUILD_DIR}/bin/build-transfer-graph" \
    "pouet1" \
    "$POLYGON_FILE"
