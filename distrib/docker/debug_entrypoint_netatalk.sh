#!/bin/sh

# Debug / profiling entry point script for the netatalk container.
# Based on entrypoint_netatalk.sh with added flamegraph profiling support.
#
# This script profiles afpd with Linux perf during test execution and
# generates an interactive SVG flamegraph. Use with debug_alp.Dockerfile.
#
# See doc/developer/profiling.md for full documentation.
#
# Copyright (C) 2023  Eric Harmon
# Copyright (C) 2024-2026  Daniel Markstedt <daniel@mindani.net>
# Copyright (C) 2025-2026  Andy Lemin (@andylemin)
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

# --------------------------------------------------------------------------
# When FLAMEGRAPH is set, we redirect all script output (echo, command
# output, test logs) to stderr so that stdout is reserved exclusively
# for the SVG flamegraph.  This allows capturing the SVG cleanly:
#
#   docker run ... > flamegraph.svg
#
# All progress/log messages still appear on the terminal via stderr.
# --------------------------------------------------------------------------
if [ -n "$FLAMEGRAPH" ]; then
    exec 3>&1 # save original stdout as fd 3 (SVG will go here)
    exec 1>&2 # redirect stdout → stderr (all echo/logs go to terminal)
fi

echo "*** Setting up environment (debug/profiling entrypoint)"

# Exit immediately on error
set -e

[ -n "$DEBUG_ENTRY_SCRIPT" ] && set -x

# --------------------------------------------------------------------------
# Common environment setup (users, volumes, config, services)
# --------------------------------------------------------------------------

. /env_setup.sh

# --------------------------------------------------------------------------
# Flamegraph profiling helper functions
# --------------------------------------------------------------------------

FLAMEGRAPH_DIR="/opt/FlameGraph"
PERF_DATA="/tmp/perf.data"
PERF_SCRIPT="/tmp/perf.script"
PERF_FOLDED="/tmp/perf.folded"
FLAMEGRAPH_SVG="/tmp/flamegraph.svg"
PERF_PID=""

# Perf sampling frequency (samples per second). Default 999 for high
# resolution profiling. Use a prime-ish number to avoid lockstep with
# timer interrupts.
PERF_FREQ="${PERF_FREQ:-999}"

start_flamegraph_profiling() {
    # Verify FlameGraph tools are available
    if [ ! -f "$FLAMEGRAPH_DIR/flamegraph.pl" ]; then
        echo "WARNING: FlameGraph tools not found at $FLAMEGRAPH_DIR"
        echo "         Ensure the Dockerfile cloned https://github.com/brendangregg/FlameGraph"
        echo "         Continuing without profiling."
        return 0
    fi

    # Verify perf is available
    if ! command -v perf > /dev/null 2>&1; then
        echo "WARNING: 'perf' command not found. Install linux-perf (Debian) or perf (Alpine)."
        echo "         Continuing without profiling."
        return 0
    fi

    # Find the afpd PID(s) to profile.
    # BusyBox pgrep (Alpine) does not support -d, so we join PIDs manually.
    AFPD_PIDS=$(pgrep afpd | tr '\n' ',' | sed 's/,$//' || true)
    if [ -z "$AFPD_PIDS" ]; then
        echo "WARNING: No afpd processes found for flamegraph profiling"
        return 0
    fi

    if [ -n "$FLAMEGRAPH_OFFCPU" ]; then
        # Off-CPU flamegraph: traces scheduler context switches to show where
        # afpd spends time sleeping/blocked (I/O, poll, locks, etc).
        # Useful for I/O-bound workloads like the speedtest where afpd spends
        # most wall-time waiting rather than executing on-CPU.
        # Limit data size to 1GB to avoid exhausting CI runner disk space,
        # since off-CPU tracing records every context switch (not sampled).
        echo "*** Setting up OFF-CPU flamegraph profiling (sched:sched_switch tracepoint)"
        echo "*** Starting perf record on afpd PIDs: $AFPD_PIDS"
        perf record -e sched:sched_switch --call-graph dwarf --max-size 1G -p "$AFPD_PIDS" -o "$PERF_DATA" &
    else
        # On-CPU flamegraph: samples at PERF_FREQ Hz to show where afpd
        # spends CPU time. Best for CPU-bound workloads like spectest/lantest.
        # Use --call-graph dwarf for reliable stack unwinding on kernels
        # compiled without frame pointers (e.g. GitHub Actions x86_64 runners).
        echo "*** Setting up ON-CPU flamegraph profiling (perf sampling at ${PERF_FREQ} Hz)"
        echo "*** Starting perf record on afpd PIDs: $AFPD_PIDS"
        perf record -F "$PERF_FREQ" --call-graph dwarf -p "$AFPD_PIDS" -o "$PERF_DATA" &
    fi
    PERF_PID=$!
    echo "*** perf record started (PID: $PERF_PID)"
}

stop_flamegraph_profiling() {
    if [ -z "$PERF_PID" ]; then
        echo "*** Flamegraph: no perf process to stop"
        return 0
    fi

    echo "*** Stopping perf record (PID: $PERF_PID)"
    kill -INT "$PERF_PID" 2> /dev/null || true
    # Wait for perf to finish writing perf.data, then sync filesystem
    # buffers to ensure the data is fully on disk before we read it.
    wait "$PERF_PID" 2> /dev/null || true
    sync

    if [ ! -f "$PERF_DATA" ]; then
        echo "WARNING: $PERF_DATA not found — perf record may have failed"
        echo "         Ensure the container is run with --privileged"
        return 1
    fi

    echo "*** perf.data size: $(du -h "$PERF_DATA" | cut -f1)"
    echo "*** Generating flamegraph from perf data"

    perf script -i "$PERF_DATA" > "$PERF_SCRIPT"

    "$FLAMEGRAPH_DIR/stackcollapse-perf.pl" "$PERF_SCRIPT" > "$PERF_FOLDED"

    if [ -n "$FLAMEGRAPH_OFFCPU" ]; then
        # Off-CPU: blue color palette and distinct title to distinguish
        # from on-CPU flamegraphs.
        "$FLAMEGRAPH_DIR/flamegraph.pl" \
            --title "Netatalk afpd ${TESTSUITE} OFF-CPU flamegraph" \
            --subtitle "buildtype=debugoptimized, sched:sched_switch tracepoint" \
            --color io \
            "$PERF_FOLDED" > "$FLAMEGRAPH_SVG"
    else
        "$FLAMEGRAPH_DIR/flamegraph.pl" \
            --title "Netatalk afpd ${TESTSUITE} flamegraph" \
            --subtitle "buildtype=debugoptimized, perf @ ${PERF_FREQ} Hz" \
            "$PERF_FOLDED" > "$FLAMEGRAPH_SVG"
    fi

    echo "*** Flamegraph generated: $FLAMEGRAPH_SVG"
    echo "*** flamegraph.svg size: $(du -h "$FLAMEGRAPH_SVG" | cut -f1)"
    echo "*** Folded stacks: $(wc -l < "$PERF_FOLDED") unique stacks"

    # Extract top 10 leaf functions by self-time (samples where this
    # function is at the top of the stack, i.e. actually on-CPU).
    # Folded format: "func1;func2;leaf_func count"
    echo "*** Top 10 leaf functions (self-time):"
    awk '{
        # Split: last field is count, everything before is the stack
        count = $NF
        stack = $0
        sub(/ [0-9]+$/, "", stack)
        # Leaf = last function in semicolon-separated stack
        n = split(stack, parts, ";")
        leaf = parts[n]
        totals[leaf] += count
    }
    END {
        for (f in totals) print totals[f], f
    }' "$PERF_FOLDED" | sort -rn | head -10 | while read cnt fn; do
        echo "FLAMEGRAPH_LEAF: $cnt $fn"
    done

    # Emit the SVG to fd 3 (the original stdout, saved at script start).
    # This is the ONLY data that goes to stdout, so the user can capture
    # it cleanly with:  docker run ... > flamegraph.svg
    echo "*** Writing flamegraph SVG to stdout (fd 3)"
    cat "$FLAMEGRAPH_SVG" >&3
}

# --------------------------------------------------------------------------
# Main: start server and run tests with optional profiling
# --------------------------------------------------------------------------

echo "*** Starting AFP server"
if [ -z "$TESTSUITE" ]; then
    netatalk -d
else
    netatalk
    sleep 2

    # Start flamegraph profiling if FLAMEGRAPH env var is set
    if [ -n "$FLAMEGRAPH" ]; then
        start_flamegraph_profiling
    fi

    echo "*** Running testsuite: $TESTSUITE"
    echo ""
    # Temporarily disable exit-on-error to ensure we can dump logs even if test fails
    set +e
    set -x
    TEST_START=$(date +%s)
    TEST_EXIT_CODE=0
    case "$TESTSUITE" in
        spectest)
            afp_spectest $TEST_FLAGS -"$AFP_VERSION" -h "$AFP_HOST" -p "$AFP_PORT" -u "$AFP_USER" -d "$AFP_USER2" -w "$AFP_PASS" -s "$SHARE_NAME" -S "$SHARE_NAME2"
            TEST_EXIT_CODE=$?
            ;;
        readonly)
            echo "testfile uno" > /mnt/afpshare/first.txt
            echo "testfile dos" > /mnt/afpshare/second.txt
            mkdir /mnt/afpshare/third
            afp_spectest $TEST_FLAGS -"$AFP_VERSION" -h "$AFP_HOST" -p "$AFP_PORT" -u "$AFP_USER" -w "$AFP_PASS" -s "$SHARE_NAME" -f Readonly_test
            TEST_EXIT_CODE=$?
            ;;
        login)
            afp_logintest $TEST_FLAGS -"$AFP_VERSION" -h "$AFP_HOST" -p "$AFP_PORT" -u "$AFP_USER" -w "$AFP_PASS"
            TEST_EXIT_CODE=$?
            ;;
        lan)
            afp_lantest $TEST_FLAGS -"$AFP_VERSION" -h "$AFP_HOST" -p "$AFP_PORT" -u "$AFP_USER" -w "$AFP_PASS" -s "$SHARE_NAME"
            TEST_EXIT_CODE=$?
            ;;
        speed)
            afp_speedtest $TEST_FLAGS -"$AFP_VERSION" -h "$AFP_HOST" -p "$AFP_PORT" -u "$AFP_USER" -w "$AFP_PASS" -s "$SHARE_NAME" -n 5 -f Read,Write,Copy,ServerCopy
            TEST_EXIT_CODE=$?
            ;;
        *)
            echo "Unknown testsuite: $TESTSUITE" >&2
            TEST_EXIT_CODE=1
            ;;
    esac
    set +x
    set -e

    TEST_END=$(date +%s)
    TEST_DURATION=$((TEST_END - TEST_START))
    echo "FLAMEGRAPH_RUNTIME: ${TEST_DURATION}s"

    # Stop profiling and generate flamegraph after tests complete.
    # Use || true to prevent profiling failures from masking the test
    # exit code — if profiling breaks, we still want to report the
    # actual test result.
    if [ -n "$FLAMEGRAPH" ]; then
        stop_flamegraph_profiling || echo "WARNING: flamegraph profiling failed (exit code: $?)" >&2
    fi

    echo "==== TESTS (${TESTSUITE}) COMPLETED (exit code: $TEST_EXIT_CODE) ===="
    # Display Netatalk's server logs if SERVER_LOGS environment variable is set
    if [ -n "$SERVER_LOGS" ]; then
        if [ -f /var/log/afpd.log ]; then
            echo "/var/log/afpd.log log lines: $(wc -l /var/log/afpd.log | awk '{print $1}')"
            echo "==== AFPD LOG CONTENT ===="
            cat /var/log/afpd.log
            echo "==== AFPD LOG END ===="
        else
            echo "NOTE: /var/log/afpd.log does not exist"
        fi
    fi
    exit $TEST_EXIT_CODE
fi
