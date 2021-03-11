#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

rm -rf NOGIT_dummy_build
conan install --install-folder=NOGIT_dummy_build MyCustomUsage --profile=MyCustomUsage/conanprofile.txt
cmake -BNOGIT_dummy_build -HMyCustomUsage
make -C NOGIT_dummy_build dummy
NOGIT_dummy_build/Mains/Dummy/dummy
