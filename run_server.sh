#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

this_script_parent="$(realpath "$(dirname "$0")" )"

BUILD_DIR="$this_script_parent/_build/server"
CMAKE_ROOT_DIR="$this_script_parent/Investigations"
echo "BUILD_DIR=$BUILD_DIR"
echo "CMAKE_ROOT_DIR=$CMAKE_ROOT_DIR"

echo "To build from scratch :  rm -rf '$BUILD_DIR'"
# rm -rf "$BUILD_DIR"

pushd "$CMAKE_ROOT_DIR"
mkdir -p "$BUILD_DIR"
conan install --install-folder="$BUILD_DIR" . --profile="${CMAKE_ROOT_DIR}/conanprofile.txt"
cmake -B"$BUILD_DIR" -H"$CMAKE_ROOT_DIR"
make -j -C "$BUILD_DIR" ultra-server
popd


# the used compiler is a "local" one (system's clang is not c++17 compliant) :
CLANG_LIBS="/home/pdrabczuk/Logiciels/clang+llvm-9.0.1-x86_64-linux-gnu-ubuntu-16.04/lib"

# this binary needs that data has been built :

# run server :
WORKDIR="${this_script_parent}/WORKDIR"
echo "Using data from WORKDIR = $WORKDIR"
LD_LIBRARY_PATH="${CLANG_LIBS}" "${BUILD_DIR}/bin/ultra-server" \
    "${WORKDIR}/COMPUTE_SHORTCUTS_OUTPUT/ultra_shortcuts.binary" \
    "${WORKDIR}/BUILD_BUCKETCH_OUTPUT/bucketch.graph"
