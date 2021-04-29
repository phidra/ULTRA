#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

this_script_parent="$(realpath "$(dirname "$0")" )"


# ARG = uwpreprocessed data :
INPUT_UWPREPROCESSED_DATADIR="$(realpath "${1}" )"
echo "Using uwpreprocessed datadir = $INPUT_UWPREPROCESSED_DATADIR"
[ ! -d "$INPUT_UWPREPROCESSED_DATADIR" ] && echo "ERROR : missing INPUT uwpreprocessed datadir : $INPUT_UWPREPROCESSED_DATADIR" && exit 1


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



# === copy uwpreprocessed data :
UWPREPROCESSED_DATA="$WORKDIR/INPUT-UWPREPROCESSED"
mkdir -p "$UWPREPROCESSED_DATA"
cp -R "$INPUT_UWPREPROCESSED_DATADIR"/* "$UWPREPROCESSED_DATA"



# === building ULTRA binary data :
echo ""
echo "Building ULTRA data :"
set -o xtrace
"${BUILD_DIR}/bin/build-ultra-binary-data" \
    "$UWPREPROCESSED_DATA/gtfs.json" \
    "$UWPREPROCESSED_DATA/walking_graph.json" \
    "$WORKDIR"
