#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

this_script_parent="$(realpath "$(dirname "$0")" )"

BUILD_DIR="$this_script_parent/_build"
CMAKE_ROOT_DIR="$this_script_parent/MyCustomUsage"
echo "BUILD_DIR=$BUILD_DIR"
echo "CMAKE_ROOT_DIR=$CMAKE_ROOT_DIR"

echo "To build from scratch :  rm -rf '$BUILD_DIR'"
# rm -rf "$BUILD_DIR"

mkdir -p "$BUILD_DIR"
conan install --install-folder="$BUILD_DIR" "$CMAKE_ROOT_DIR" --profile="$CMAKE_ROOT_DIR/conanprofile.txt"
cmake -B"$BUILD_DIR" -H"$CMAKE_ROOT_DIR"
make -j -C "$BUILD_DIR" ultra-server


# run server :
WORKDIR="${this_script_parent}/WORKDIR_IDF"
PORT="8080"
echo "Using data from WORKDIR = $WORKDIR"
"${BUILD_DIR}/bin/ultra-server" \
    "$PORT" \
    "${WORKDIR}/COMPUTE_SHORTCUTS_OUTPUT/ultra_shortcuts.binary" \
    "${WORKDIR}/BUILD_BUCKETCH_OUTPUT/bucketch.graph"
