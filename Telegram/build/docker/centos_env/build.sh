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

# Ninja runs compiles in parallel, so a live-streamed ccache log
# interleaves unreadably with "Building CXX object" lines and loses
# the file<->result correlation. Instead, log everything to a file
# and print a clean "file: RESULT" report after the build, grouped
# by ccache's own per-invocation PID.
export CCACHE_LOGFILE=/tmp/ccache.log
: > "$CCACHE_LOGFILE"

cmake --build ../out --config "${CONFIG:-Release}"

echo "=== ccache per-file results ==="
python3 - "$CCACHE_LOGFILE" <<'PYEOF'
import re
import sys

path = sys.argv[1]
pid_file = {}
pid_result = {}
order = []

line_re = re.compile(r"^\[[^\]]*\s(\d+)\]\s(.*)$")
compile_re = re.compile(r"-c\s+(\S+)\s")

with open(path, "r", errors="replace") as f:
    for line in f:
        m = line_re.match(line)
        if not m:
            continue
        pid, rest = m.group(1), m.group(2)
        if rest.startswith("Command line:"):
            cm = compile_re.search(rest)
            if cm:
                pid_file[pid] = cm.group(1)
                if pid not in pid_result:
                    order.append(pid)
        elif rest.startswith("Result:"):
            pid_result[pid] = rest[len("Result:"):].strip()
            if pid not in pid_file:
                order.append(pid)

hits = misses = other = 0
for pid in order:
    src = pid_file.get(pid, "?")
    result = pid_result.get(pid, "?")
    if "hit" in result:
        hits += 1
        continue
    elif result == "cache_miss":
        misses += 1
    else:
        other += 1
    # Only print non-hits - hits are the expected case and would
    # otherwise flood the log; misses/disabled are what's worth seeing.
    print(f"{result:24s} {src}")

print(f"--- {hits} hits, {misses} misses, {other} other/disabled ({len(order)} total) ---")
PYEOF

echo "=== ccache stats after build ==="
ccache -s
