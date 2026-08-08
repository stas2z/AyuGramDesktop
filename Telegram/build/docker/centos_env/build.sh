#!/bin/bash
set -e

cd Telegram
./configure.sh "$@"

# Force lang subsets regeneration by removing timestamp
rm -f ../out/Telegram/gen/lang_subsets.timestamp

# Use ccache for faster incremental builds
export CC="ccache gcc"
export CXX="ccache g++"
export CCACHE_COMPRESS=1
export CCACHE_COMPRESSLEVEL=6
export CCACHE_MAXSIZE=10G

cmake --build ../out --config "${CONFIG:-Release}"
