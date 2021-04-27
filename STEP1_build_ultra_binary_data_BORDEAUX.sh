#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

this_script_parent="$(realpath "$(dirname "$0")" )"


# ARG = preparatory data :
INPUT_PREPARATORY_DATADIR="$(realpath "${1}" )"
echo "Using preparatory datadir = $INPUT_PREPARATORY_DATADIR"
[ ! -d "$INPUT_PREPARATORY_DATADIR" ] && echo "ERROR : missing INPUT preparatory datadir" && exit 1


# WORKDIR :
WORKDIR="${this_script_parent}/WORKDIR_build_ultra_binary_data_BORDEAUX"


# === building
BUILD_DIR="$this_script_parent/_build"
CMAKE_ROOT_DIR="$this_script_parent/MyCustomUsage"
echo "BUILD_DIR=$BUILD_DIR"
echo "CMAKE_ROOT_DIR=$CMAKE_ROOT_DIR"
echo "To build from scratch :  rm -rf '$BUILD_DIR'"

mkdir -p "$BUILD_DIR"
conan install --install-folder="$BUILD_DIR" "$CMAKE_ROOT_DIR" --profile="$CMAKE_ROOT_DIR/conanprofile.txt"
CXX=$(which clang++) cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo  -B"$BUILD_DIR" -H"$CMAKE_ROOT_DIR"
make -j -C "$BUILD_DIR" build-ultra-binary-data



# === copy preparatory data :
PREPARED_DATA="$WORKDIR/INPUT-PREPARATORY"
mkdir -p "$PREPARED_DATA"
cp -R "$INPUT_PREPARATORY_DATADIR"/* "$PREPARED_DATA"



# === building ULTRA binary data :
echo ""
echo "Building ULTRA data :"
set -o xtrace
"${BUILD_DIR}/bin/build-ultra-binary-data" \
    "$PREPARED_DATA/gtfs.json" \
    "$PREPARED_DATA/walking_graph.json" \
    "$WORKDIR"
