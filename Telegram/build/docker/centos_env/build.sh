#!/bin/bash
set -e

cd Telegram
./configure.sh "$@"

# Use ccache for faster incremental builds
export CC="ccache gcc"
export CXX="ccache g++"
export CCACHE_COMPRESS=1
export CCACHE_COMPRESSLEVEL=6
export CCACHE_MAXSIZE=10G

echo "=== ccache stats before build ==="
ccache -s

# Stream a per-file HIT/MISS line into the build log as it happens,
# so it's visible right alongside each "Building CXX object" line
# instead of only as a final aggregate count.
export CCACHE_LOGFILE=/tmp/ccache.log
: > "$CCACHE_LOGFILE"
(
	tail -F -n0 "$CCACHE_LOGFILE" 2>/dev/null \
		| grep --line-buffered -E "Result:|^Compiled" \
		| sed -u 's/^.*\] //'
) &
CCACHE_TAIL_PID=$!
trap 'kill "$CCACHE_TAIL_PID" 2>/dev/null || true' EXIT

cmake --build ../out --config "${CONFIG:-Release}"

kill "$CCACHE_TAIL_PID" 2>/dev/null || true

echo "=== ccache stats after build ==="
ccache -s
