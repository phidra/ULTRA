#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

this_script_parent="$(realpath "$(dirname "$0")" )"

BUILD_DIR="$this_script_parent/Analyzer/_build"
CMAKE_ROOT_DIR="$this_script_parent/Analyzer"
echo "BUILD_DIR=$BUILD_DIR"
echo "CMAKE_ROOT_DIR=$CMAKE_ROOT_DIR"

echo "To build from scratch :  rm -rf '$BUILD_DIR'"
rm -rf "$BUILD_DIR"

pushd "$CMAKE_ROOT_DIR"
mkdir -p "$BUILD_DIR"
conan install --install-folder="$BUILD_DIR" . --profile="${CMAKE_ROOT_DIR}/conanprofile.txt"
cmake -B"$BUILD_DIR" -H"$CMAKE_ROOT_DIR"
make -j -C "$BUILD_DIR"
popd


# the used compiler is a "local" one (system's clang is not c++17 compliant) :
CLANG_LIBS="/home/pdrabczuk/Logiciels/clang+llvm-9.0.1-x86_64-linux-gnu-ubuntu-16.04/lib"

# download data :
"${this_script_parent}"/download_KIT_data.sh
DOWNLOADED_DATA="${this_script_parent}/DATA/complete"


# run analyzer :
WORKDIR="${this_script_parent}/WORKDIR_analyze_binary"
echo "Using WORKDIR = $WORKDIR"
mkdir -p "$WORKDIR"
cp -R "${DOWNLOADED_DATA}/"* "${WORKDIR}"
LD_LIBRARY_PATH="${CLANG_LIBS}" Analyzer/_build/bin/binary-analyzer "${WORKDIR}/raptor.binary"
