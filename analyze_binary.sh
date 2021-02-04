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
conan install --install-folder="$BUILD_DIR" .
cmake -B"$BUILD_DIR" -H"$CMAKE_ROOT_DIR"
make -j -C "$BUILD_DIR" ultra-binary-analyzer
popd

# download data :
"${this_script_parent}"/download_KIT_data.sh
DOWNLOADED_DATA="${this_script_parent}/DOWNLOADED_DATA/complete"


# run analyzer :
WORKDIR="${this_script_parent}/WORKDIR_analyze_binary"
echo "Using WORKDIR = $WORKDIR"
mkdir -p "$WORKDIR"
cp -R "${DOWNLOADED_DATA}/"* "${WORKDIR}"
"${BUILD_DIR}/bin/ultra-binary-analyzer" "${WORKDIR}/raptor.binary"
