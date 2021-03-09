#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

this_script_parent="$(realpath "$(dirname "$0")" )"



# if needed :
# make -C Runnables clean

# build :
make -C Runnables

# working directory :
WORKDIR="${this_script_parent}/WORKDIR_BORDEAUX"
echo "Using WORKDIR = $WORKDIR"
mkdir -p "$WORKDIR"

# STEP 0 = input data :
#==========
INPUT_DATA="${this_script_parent}/WORKDIR_build_ultra_binary_data_BORDEAUX"


# STEP 1 = BuildCoreCH 
#==========
echo ""
echo "=== RUNNING BuildCoreCH"
BUILD_CORE_CH_INPUT_DIR="${WORKDIR}/INPUT_DATA"
BUILD_CORE_CH_OUTPUT_DIR="${WORKDIR}/BUILD_CORE_CH_OUTPUT"
mkdir -p "${BUILD_CORE_CH_OUTPUT_DIR}"
rm -rf "${BUILD_CORE_CH_INPUT_DIR}"
cp -R "${INPUT_DATA}" "${BUILD_CORE_CH_INPUT_DIR}"
CORE_DEGREE=14
Runnables/BuildCoreCH  \
    "${BUILD_CORE_CH_INPUT_DIR}/raptor.binary" \
    "${CORE_DEGREE}" \
    "${BUILD_CORE_CH_OUTPUT_DIR}/"


# STEP 2 = ComputeShortcuts
#==========
echo ""
echo "=== RUNNING ComputeShortcuts"
COMPUTE_SHORTCUTS_INPUT_DIR="${BUILD_CORE_CH_OUTPUT_DIR}"
COMPUTE_SHORTCUTS_OUTPUT_DIR="${WORKDIR}/COMPUTE_SHORTCUTS_OUTPUT"
COMPUTE_SHORTCUTS_OUTPUT_FILENAME="${COMPUTE_SHORTCUTS_OUTPUT_DIR}/ultra_shortcuts.binary"
mkdir -p "${COMPUTE_SHORTCUTS_OUTPUT_DIR}"
TRANSFER_LIMIT=$((15*60))
NB_THREADS=4
PIN_MULTIPLIER=1
REQUIRE_DIRECT_TRANSFERS="true"
Runnables/ComputeShortcuts \
    "${COMPUTE_SHORTCUTS_INPUT_DIR}/raptor.binary" \
    "${TRANSFER_LIMIT}" \
    "${COMPUTE_SHORTCUTS_OUTPUT_FILENAME}" \
    "${NB_THREADS}" \
    "${PIN_MULTIPLIER}" \
    "${REQUIRE_DIRECT_TRANSFERS}"

# STEP 3 = BuildBucketCH
#==========
echo ""
echo "=== RUNNING BuildBucketCH"
BUILD_BUCKETCH_INPUT_DIR="${BUILD_CORE_CH_OUTPUT_DIR}"
BUILD_BUCKETCH_OUTPUT_DIR="${WORKDIR}/BUILD_BUCKETCH_OUTPUT"
BUILD_BUCKETCH_OUTPUT_FILENAME="${BUILD_BUCKETCH_OUTPUT_DIR}/bucketch.graph"
mkdir -p "${BUILD_BUCKETCH_OUTPUT_DIR}"
Runnables/BuildBucketCH \
    "${BUILD_BUCKETCH_INPUT_DIR}/raptor.binary.graph" \
    "${BUILD_BUCKETCH_OUTPUT_FILENAME}"