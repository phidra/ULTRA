#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail


echo ""
echo "USAGE:      $0  </path/to/ultra-binary-data>      <port>"
echo "EXAMPLE:    $0  ULTRA_DATA_bordeaux               8080"
echo "EXAMPLE:    $0  ULTRA_DATA_idf                    8080"

echo ""


# ARG (mandatory thanks to nounset) = input data :
INPUT_DATA="$(realpath "${1}" )"
echo "Using input data= $INPUT_DATA"
[ ! -d "$INPUT_DATA" ] && echo "ERROR : missing INPUT DATA : $INPUT_DATA" && exit 1


# ARG (mandatory thanks to nounset) = server port :
SERVER_PORT="$2"
echo "Serving on port = $SERVER_PORT"



# building :
this_script_parent="$(realpath "$(dirname "$0")" )"
BUILD_DIR="$this_script_parent/_build"
CMAKE_ROOT_DIR="$this_script_parent/MyCustomUsage"
echo "BUILD_DIR=$BUILD_DIR"
echo "CMAKE_ROOT_DIR=$CMAKE_ROOT_DIR"

echo "To build from scratch :  rm -rf '$BUILD_DIR'"
# rm -rf "$BUILD_DIR"

mkdir -p "$BUILD_DIR"
conan install --install-folder="$BUILD_DIR" "$CMAKE_ROOT_DIR" --profile="$CMAKE_ROOT_DIR/conanprofile.txt"
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo  -B"$BUILD_DIR" -H"$CMAKE_ROOT_DIR"
make -j -C "$BUILD_DIR" ultra-server


# run server :
"${BUILD_DIR}/bin/ultra-server" \
    "$SERVER_PORT" \
    "${INPUT_DATA}/COMPUTE_SHORTCUTS_OUTPUT/ultra_shortcuts.binary" \
    "${INPUT_DATA}/BUILD_BUCKETCH_OUTPUT/bucketch.graph"
