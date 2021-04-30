#!/bin/bash

set -o nounset
set -o errexit
set -o pipefail


echo ""
echo "USAGE:      $0  </path/to/uwpreprocessed-data>      <workdir>"
echo "EXAMPLE:    $0  NOGIT_uwpreprocessed_data_BORDEAUX  WORKDIR_bordeaux"
echo "EXAMPLE:    $0  NOGIT_uwpreprocessed_data_IDF       WORKDIR_idf"

echo ""



# ARG (mandatory thanks to nounset) = uwpreprocessed data :
INPUT_UWPREPROCESSED_DATADIR="$(realpath "${1}" )"
echo "Using uwpreprocessed datadir = $INPUT_UWPREPROCESSED_DATADIR"
[ ! -d "$INPUT_UWPREPROCESSED_DATADIR" ] && echo "ERROR : missing INPUT uwpreprocessed datadir : $INPUT_UWPREPROCESSED_DATADIR" && exit 1


# ARG (mandatory thanks to nounset) = working directory (must not exist) :
WORKDIR="$(realpath "${2}" )"
echo "Using WORKDIR = $WORKDIR"
[ -e "$WORKDIR" ] && echo "ERROR : already existing workdir :  $WORKDIR" && exit 1
mkdir -p "$WORKDIR"


# === building
this_script_parent="$(realpath "$(dirname "$0")" )"
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
INPUT_GTFS="$UWPREPROCESSED_DATA/gtfs.json"
INPUT_GRAPH="$UWPREPROCESSED_DATA/walking_graph.json"
[ ! -f "$INPUT_GTFS" ] && echo "ERROR : missing INPUT uwpreprocessed gtsf : $INPUT_GTFS" && exit 1
[ ! -f "$INPUT_GRAPH" ] && echo "ERROR : missing INPUT uwpreprocessed graph : $INPUT_GRAPH" && exit 1



# === building ULTRA binary data :
echo ""
echo "Building ULTRA data :"
set -o xtrace
"${BUILD_DIR}/bin/build-ultra-binary-data" \
    "$INPUT_GTFS" \
    "$INPUT_GRAPH" \
    "$WORKDIR"
