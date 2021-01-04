#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

this_script_parent="$(realpath "$(dirname "$0")" )"



# if needed :
# make -C Runnables clean

# build :
make -C Runnables

# the used compiler is a "local" one (system's clang is not c++17 compliant) :
CLANG_INSTALL_DIR="/home/pdrabczuk/Logiciels/clang+llvm-9.0.1-x86_64-linux-gnu-ubuntu-16.04"
CLANG_LIBS="${CLANG_INSTALL_DIR}/lib"


# working directory :
WORKDIR="${this_script_parent}/WORKDIR"
echo "Using WORKDIR = $WORKDIR"
mkdir -p "$WORKDIR"

cat << EOF

WARNING : for now, parameters are just "guessed", i.e. they are not particularly chosen :
The first value that can run the binary is used.

NOTE : rough time needed to run pipeline on KIT's complete test data :

STEP 1 = BuildCoreCH
    Building CH took 2m 5s 639ms
STEP 2 = ComputeShortcuts
    Took 1h 34m 27s 111ms
STEP 3 = BuildBucketCH
    Building CH took 1m 44s 36ms
STEP 4 = RunRAPTORQueries (pour 10 queries)
    ~10 secondes (surtout le temps de charger les graphes)

EOF

# STEP 0 = download data :
#==========
"${this_script_parent}"/download_KIT_data.sh
DOWNLOADED_DATA="${this_script_parent}/DATA/complete"


# STEP 1 = BuildCoreCH 
#==========
BUILD_CORE_CH_INPUT_DIR="${WORKDIR}/INPUT_DATA"
BUILD_CORE_CH_OUTPUT_DIR="${WORKDIR}/BUILD_CORE_CH_OUTPUT"
mkdir -p "${BUILD_CORE_CH_OUTPUT_DIR}"
rm -rf "${BUILD_CORE_CH_INPUT_DIR}"
cp -R "${DOWNLOADED_DATA}" "${BUILD_CORE_CH_INPUT_DIR}"
LD_LIBRARY_PATH="${CLANG_LIBS}" Runnables/BuildCoreCH  \
    "${BUILD_CORE_CH_INPUT_DIR}/raptor.binary" \
    14 \
    "${BUILD_CORE_CH_OUTPUT_DIR}/"


# STEP 2 = ComputeShortcuts
#==========
COMPUTE_SHORTCUTS_INPUT_DIR="${BUILD_CORE_CH_OUTPUT_DIR}"
COMPUTE_SHORTCUTS_OUTPUT_DIR="${WORKDIR}/COMPUTE_SHORTCUTS_OUTPUT"
COMPUTE_SHORTCUTS_OUTPUT_FILENAME="${COMPUTE_SHORTCUTS_OUTPUT_DIR}/ultra_shortcuts.binary"
mkdir -p "${COMPUTE_SHORTCUTS_OUTPUT_DIR}"
LD_LIBRARY_PATH="${CLANG_LIBS}" Runnables/ComputeShortcuts \
    "${COMPUTE_SHORTCUTS_INPUT_DIR}/raptor.binary" \
    $((15*60)) \
    "${COMPUTE_SHORTCUTS_OUTPUT_FILENAME}" \
    4 \
    1 \
    true

# STEP 3 = BuildBucketCH
#==========
BUILD_BUCKETCH_INPUT_DIR="${BUILD_CORE_CH_OUTPUT_DIR}"
BUILD_BUCKETCH_OUTPUT_DIR="${WORKDIR}/BUILD_BUCKETCH_OUTPUT"
BUILD_BUCKETCH_OUTPUT_FILENAME="${BUILD_BUCKETCH_OUTPUT_DIR}/bucketch.graph"
mkdir -p "${BUILD_BUCKETCH_OUTPUT_DIR}"
LD_LIBRARY_PATH="${CLANG_LIBS}" Runnables/BuildBucketCH \
    "${BUILD_BUCKETCH_INPUT_DIR}/raptor.binary.graph" \
    "${BUILD_BUCKETCH_OUTPUT_FILENAME}"

# STEP 4 = RunRAPTORQueries
#==========
NB_QUERIES=10
SEED=42
RAPTOR_QUERIES_OUTPUT_FILE="${WORKDIR}/RAPTOR_queries.csv"
LD_LIBRARY_PATH="${CLANG_LIBS}" Runnables/RunRAPTORQueries \
    shortcuts \
    "${COMPUTE_SHORTCUTS_OUTPUT_FILENAME}" \
    $NB_QUERIES \
    $SEED \
    "${RAPTOR_QUERIES_OUTPUT_FILE}" \
    "${BUILD_BUCKETCH_OUTPUT_FILENAME}"
