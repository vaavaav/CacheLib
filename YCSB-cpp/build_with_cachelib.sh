#!/bin/sh

# Copyright (c) Facebook, Inc. and its affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -e

# Root directory for the CacheLib project
CLBASE="$PWD/.."

# Additional "FindXXX.cmake" files are here (e.g. FindSodium.cmake)
CACHELIBCMAKE="$CLBASE/cachelib/cmake"

# After ensuring we are in the correct directory, set the installation prefix"
CACHELIB="$CLBASE/opt/cachelib"

CMAKE_PARAMS="-DCMAKE_INSTALL_PREFIX='$CACHELIB' -DCMAKE_MODULE_PATH='$CACHELIBCMAKE' -DCMAKE_BUILD_TYPE=Debug -DBIND_CACHELIB=1
-DBIND_ROCKSDB=1
"

CMAKE_PREFIX_PATH="$CACHELIB/lib/cmake:$CACHELIB/lib64/cmake:$CACHELIB/lib:$CACHELIB/lib64:$CACHELIB:${CMAKE_PREFIX_PATH:-}"
export CMAKE_PREFIX_PATH
PKG_CONFIG_PATH="$CACHELIB/lib/pkgconfig:$CACHELIB/lib64/pkgconfig:${PKG_CONFIG_PATH:-}"
export PKG_CONFIG_PATH
LD_LIBRARY_PATH="lib64:$CACHELIB/lib:$CACHELIB/lib64:${LD_LIBRARY_PATH:-}"
export LD_LIBRARY_PATH

mkdir -p build
cd build
cmake $CMAKE_PARAMS ..
make 
