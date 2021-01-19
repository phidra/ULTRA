#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

this_script_parent="$(realpath "$(dirname "$0")" )"

BUILD_DIR="$this_script_parent/Investigations/_build_converter"
CMAKE_ROOT_DIR="$this_script_parent/Investigations"
echo "BUILD_DIR=$BUILD_DIR"
echo "CMAKE_ROOT_DIR=$CMAKE_ROOT_DIR"

echo "To build from scratch :  rm -rf '$BUILD_DIR'"
# rm -rf "$BUILD_DIR"


# the used compiler is a "local" one (system's clang is not c++17 compliant) :
CLANG_LIBS="/home/pdrabczuk/Logiciels/clang+llvm-9.0.1-x86_64-linux-gnu-ubuntu-16.04/lib"



# Download cppgtfs library :
LIBS_DIR="${this_script_parent}/libs"
CPPGTFS_LIB_DIR="${LIBS_DIR}/cppgtfs/"
CPPGTFS_URL="https://ad-git.informatik.uni-freiburg.de/ad/cppgtfs/-/archive/master/cppgtfs-master.zip"

if [ ! -d "${CPPGTFS_LIB_DIR}" ]
then
    mkdir -p "${CPPGTFS_LIB_DIR}"
    TEMP_CPPGTFS_DIR="$(mktemp -d --suffix "_CPPGTFS_DOWNLOAD")"
    pushd "${TEMP_CPPGTFS_DIR}"
    echo "About to download cppgtfs library : ${CPPGTFS_URL}"
    wget "${CPPGTFS_URL}"
    unzip cppgtfs-master.zip
    mv cppgtfs-master/src/* "${CPPGTFS_LIB_DIR}"
    popd
    rm -rf "${TEMP_CPPGTFS_DIR}"

    # until I submit a PR with a fix, cppgtfs needs to be patched to rename pow10 :
    sed -i "s@pow10@powersOf10@g" "${CPPGTFS_LIB_DIR}/ad/util/CsvParser.cpp"

else
    echo "Not downloading cppgtfs library : already existing target dir : ${CPPGTFS_LIB_DIR}"
fi

# building :
pushd "$CMAKE_ROOT_DIR"
mkdir -p "$BUILD_DIR"
conan install --install-folder="$BUILD_DIR" . --profile="${CMAKE_ROOT_DIR}/conanprofile.txt"
cmake -B"$BUILD_DIR" -H"$CMAKE_ROOT_DIR"
#make -j -C "$BUILD_DIR" ultra-converter
make -j -C "$BUILD_DIR"
popd


# download sample GTFS data :
SAMPLE_GTFS_URL="https://github.com/google/transit/raw/master/gtfs/spec/en/examples/sample-feed-1.zip"
SAMPLE_GTFS_DIR="${this_script_parent}/DATA/gtfs_sample"
if [ ! -d "${SAMPLE_GTFS_DIR}" ]
then
    mkdir -p "${SAMPLE_GTFS_DIR}"
    TEMP_GTFS_SAMPLE_DIR="$(mktemp -d --suffix "_SAMPLE_GTFS_DOWNLOAD")"
    pushd "${TEMP_GTFS_SAMPLE_DIR}"
    echo "About to download SAMPLE gtfs data : ${SAMPLE_GTFS_URL}"
    wget "${SAMPLE_GTFS_URL}"
    unzip sample-feed-1.zip
    mv ./*.txt "${SAMPLE_GTFS_DIR}"
    popd
    rm -rf "${TEMP_GTFS_SAMPLE_DIR}"
else
    echo "Not downloading SAMPLE gtfs data : already existing target dir : ${SAMPLE_GTFS_DIR}"
fi


# run converter :
WORKDIR="${this_script_parent}/WORKDIR_converter"
echo "Using WORKDIR = $WORKDIR"
mkdir -p "$WORKDIR"
cp -R "${SAMPLE_GTFS_DIR}"/*.txt "${WORKDIR}"
LD_LIBRARY_PATH="${CLANG_LIBS}" Investigations/_build_converter/bin/ultra-converter "${WORKDIR}"
