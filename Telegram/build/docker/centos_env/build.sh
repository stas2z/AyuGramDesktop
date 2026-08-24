#!/bin/bash
set -e

cd Telegram
./configure.sh "$@"

# ccache is wired in via CMAKE_C_COMPILER_LAUNCHER/CMAKE_CXX_COMPILER_LAUNCHER
# (passed as configure flags, see build-linux.yml) rather than CC/CXX env
# vars - CMAKE_C_COMPILER/CMAKE_CXX_COMPILER are cached CMake variables only
# resolved on the *first* configure of a build tree, so once out/ is
# restored from a previous run's cache, CC/CXX overrides here would be
# silently ignored and every compile would bypass ccache entirely.
export CCACHE_COMPRESS=1
export CCACHE_COMPRESSLEVEL=6
export CCACHE_MAXSIZE=10G

# The centos_env docker image itself ships `ENV CCACHE_DISABLE=true`
# (confirmed via `docker inspect`) - environment variables always take
# priority over ccache.conf/--set-config, so no config-file-level fix
# could ever override it; every compile was silently a no-op ("Result:
# disabled") no matter what we did to the config. Unset it so ccache
# actually runs.
unset CCACHE_DISABLE
ccache --set-config=disable=false
echo "=== ccache config ==="
ccache -p | grep -E "disable|cache_dir" || true

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
pid_reason = {}
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
        elif "isabl" in rest and pid not in pid_reason:
            pid_reason[pid] = rest

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
    reason = pid_reason.get(pid, "")
    print(f"{result:24s} {src}" + (f"  ({reason})" if reason else ""))

print(f"--- {hits} hits, {misses} misses, {other} other/disabled ({len(order)} total) ---")
PYEOF

echo "=== ccache stats after build ==="
ccache -s
