#!/bin/bash
set -e

cd Telegram
./configure.sh "$@"

# Re-configure to pick up changes in lang.strings / CMakeLists.txt
cmake .. "$@"

# Use ccache for faster incremental builds
export CC="ccache gcc"
export CXX="ccache g++"
export CCACHE_COMPRESS=1
export CCACHE_COMPRESSLEVEL=6
export CCACHE_MAXSIZE=10G

cmake --build ../out --config "${CONFIG:-Release}"
