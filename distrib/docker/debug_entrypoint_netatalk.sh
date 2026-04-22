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

# Perf sampling frequency (samples per second). Default 1009 Hz — a
# prime number chosen to avoid lockstep synchronization with timer
# interrupts and the idle worker's 1ms polling interval.
PERF_FREQ="${PERF_FREQ:-1009}"

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
        perf record --inherit -e sched:sched_switch --call-graph dwarf --max-size 1G -p "$AFPD_PIDS" -o "$PERF_DATA" &
    elif [ -n "$FLAMEGRAPH_FP" ]; then
        # On-CPU flamegraph with frame-pointer unwinding: lighter weight but
        # requires -fno-omit-frame-pointer (set in debug_alp.Dockerfile).
        # Kernel/musl frames without frame pointers will show as [unknown].
        echo "*** Setting up ON-CPU flamegraph profiling (perf @ ${PERF_FREQ} Hz, frame-pointer unwinding)"
        echo "*** Starting perf record on afpd PIDs: $AFPD_PIDS"
        perf record --inherit -F "$PERF_FREQ" --call-graph fp -p "$AFPD_PIDS" -o "$PERF_DATA" &
    else
        # Default: On-CPU flamegraph with DWARF unwinding for deep stack
        # traces into kernel and musl libc. Consecutive __clone frames are
        # collapsed in post-processing. --inherit ensures forked child
        # session processes are profiled too.
        echo "*** Setting up ON-CPU flamegraph profiling (perf @ ${PERF_FREQ} Hz, DWARF unwinding)"
        echo "*** Starting perf record on afpd PIDs: $AFPD_PIDS"
        perf record --inherit -F "$PERF_FREQ" --call-graph dwarf -p "$AFPD_PIDS" -o "$PERF_DATA" &
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

    # Collapse consecutive duplicate __clone frames caused by DWARF unwinding
    # through musl's __clone assembly trampoline (common on x86_64 CI runners).
    # e.g. "afpd;__clone;__clone;__clone;start;..." → "afpd;__clone;start;..."
    perl -pi -e 's/;__clone(?:;__clone)+/;__clone/g' "$PERF_FOLDED"

    FLAMEGRAPH_DATE=$(date -u '+%Y-%m-%d %H:%M UTC')

    if [ -n "$FLAMEGRAPH_OFFCPU" ]; then
        # Off-CPU: blue color palette and distinct title to distinguish
        # from on-CPU flamegraphs.
        "$FLAMEGRAPH_DIR/flamegraph.pl" \
            --title "Netatalk afpd ${TESTSUITE} OFF-CPU flamegraph ${FLAMEGRAPH_DATE}" \
            --subtitle "buildtype=debugoptimized, sched:sched_switch tracepoint" \
            --color io \
            "$PERF_FOLDED" > "$FLAMEGRAPH_SVG"
    else
        if [ -n "$FLAMEGRAPH_FP" ]; then
            UNWIND_METHOD="frame-pointer unwinding"
        else
            UNWIND_METHOD="DWARF unwinding"
        fi
        "$FLAMEGRAPH_DIR/flamegraph.pl" \
            --title "Netatalk afpd ${TESTSUITE} flamegraph ${FLAMEGRAPH_DATE}" \
            --subtitle "buildtype=debugoptimized, perf @ ${PERF_FREQ} Hz, ${UNWIND_METHOD}" \
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

    # Calculate Netatalk self-time: percentage of on-CPU samples where the
    # leaf function (the function actually executing) is Netatalk code.
    # This measures time spent IN Netatalk functions, excluding time in
    # kernel, libc, or library functions called by Netatalk.
    #
    # Method: extract function symbols from installed Netatalk binaries
    # via nm, then match leaf functions in folded stacks against that set.
    NETATALK_SYMS="/tmp/netatalk_syms.txt"
    {
        # All Netatalk binaries (afpd is the profiled process)
        nm -D /usr/local/sbin/netatalk 2> /dev/null
        nm -D /usr/local/sbin/afpd 2> /dev/null
        nm -D /usr/local/sbin/cnid_dbd 2> /dev/null
        nm -D /usr/local/sbin/cnid_metad 2> /dev/null
        # Shared library and UAM modules
        nm -D /usr/local/lib/libatalk.so* 2> /dev/null
        nm -D /usr/local/lib/netatalk/*.so 2> /dev/null
    } | awk '/^[0-9a-f]+ [TtWw] / { print $3 }' | sort -u > "$NETATALK_SYMS"

    NETATALK_SYM_COUNT=$(wc -l < "$NETATALK_SYMS")
    echo "*** Netatalk symbol table: $NETATALK_SYM_COUNT functions"

    if [ "$NETATALK_SYM_COUNT" -gt 0 ]; then
        SELF_TIME_RESULT=$(awk -v symfile="$NETATALK_SYMS" '
            BEGIN {
                while ((getline sym < symfile) > 0) netatalk_syms[sym] = 1
                close(symfile)
            }
            {
                count = $NF
                stack = $0
                sub(/ [0-9]+$/, "", stack)
                n = split(stack, parts, ";")
                leaf = parts[n]
                total += count
                if (leaf in netatalk_syms) netatalk_self += count
            }
            END {
                if (total > 0) {
                    pct = (netatalk_self / total) * 100
                    printf "%.1f %d %d", pct, netatalk_self, total
                } else {
                    printf "0.0 0 0"
                }
            }
        ' "$PERF_FOLDED")

        SELF_PCT=$(echo "$SELF_TIME_RESULT" | cut -d' ' -f1)
        SELF_SAMPLES=$(echo "$SELF_TIME_RESULT" | cut -d' ' -f2)
        TOTAL_SAMPLES=$(echo "$SELF_TIME_RESULT" | cut -d' ' -f3)
        echo "*** Netatalk self-time: ${SELF_PCT}% (${SELF_SAMPLES}/${TOTAL_SAMPLES} leaf samples)"
        echo "FLAMEGRAPH_SELF_TIME: ${SELF_PCT}%"
    fi

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

    echo "*** AFP server compile-time features:"
    afpd -V
    echo ""

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
