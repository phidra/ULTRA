#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

this_script_parent="$(realpath "$(dirname "$0")" )"

BUILD_DIR="$this_script_parent/_build/checker"
CMAKE_ROOT_DIR="$this_script_parent/Investigations"
echo "BUILD_DIR=$BUILD_DIR"
echo "CMAKE_ROOT_DIR=$CMAKE_ROOT_DIR"

echo "To build from scratch :  rm -rf '$BUILD_DIR'"
# rm -rf "$BUILD_DIR"

# building :
pushd "$CMAKE_ROOT_DIR"
mkdir -p "$BUILD_DIR"
conan install --install-folder="$BUILD_DIR" .
cmake -B"$BUILD_DIR" -H"$CMAKE_ROOT_DIR"
make -j -C "$BUILD_DIR" gtfs-checker
popd

# run checker :
"${BUILD_DIR}/bin/gtfs-checker" "DOWNLOADED_DATA/gtfs_bordeaux"
