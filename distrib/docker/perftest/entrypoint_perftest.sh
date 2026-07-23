#!/bin/sh
# Entrypoint for the netatalk performance test image.
#
# Starts afpd in the container's root network namespace via the standard
# testsuite entrypoint, builds an isolated client network namespace joined
# by a rate-shaped veth pair, then runs the CI perfgraph test across the
# shaped link.
#
# For SUITE=speed every sweep size runs as its own afp_speedtest
# invocation so network statistics are exact per size: interface packet
# and byte counters (both directions) are read from the client veth
# before and after each invocation.  The per-size CSV fragments are then
# merged into one CSV identical in layout to a single CI sweep run;
# network statistics are written to a separate file, never into the CSV.
#
# The container must run with --privileged and a writable /output mount.
#
# Environment (defaults mirror the CI perfgraph jobs where applicable):
#   AFP_USER, AFP_PASS, SHARE_NAME, AFP_GROUP   required; as testsuite image
#   SUITE    speed | lan          test program to run (default: speed)
#   RATE     tc netem rate        (default: 10gbit; e.g. 100mbit, 1gbit)
#   DELAY    one-way link delay   (default: 2ms; RTT is twice this)
#   MTU      veth MTU             (default: 1500; 9000 for jumbo)
#   ITER     iterations           (default: 5, as CI AFP_TEST_ITERATIONS)
#   SWEEP    speedtest size list  (default: the CI perfgraph sweep)
#   OPS      speedtest operations (default: Read,Write,Copy — ServerCopy
#            copies server-side and never touches the shaped link)
#   QUANTUM    server quantum, bytes or K/M/G suffix e.g. 1M, 512K
#              (default: afpd's built-in 1 MiB)
#   READAHEAD  dsireadbuf scale factor  (default: afpd's built-in)
#   EXTRA_FLAGS  extra flags passed to the test program, before the fixed
#                connection args (default: empty)
#   RUN_NAME   output basename for this run (default: <suite>-<rate>)
#
# Outputs in /output, named by suite and rate (e.g. speed-1gbit):
#   <name>.csv       merged CSV, same layout as a CI sweep run
#   <name>-stderr.log  test program stderr (all invocations; empty on a
#                    clean run — the tools run with -c/quiet, so only
#                    errors and diagnostics land here)
#   <name>-net.csv   per-size network statistics (rx/tx packets, bytes,
#                    average payload per segment, both directions)
#   <name>-net.txt   run summary (MSS, totals)

set -eu

SUITE="${SUITE:-speed}"
RATE="${RATE:-10gbit}"
DELAY="${DELAY:-2ms}"
MTU="${MTU:-1500}"
ITER="${ITER:-5}"
SWEEP="${SWEEP:-0.00390625,0.0078125,0.015625,0.03125,0.0625,0.125,0.25,0.5,1,2,4,8,16,32,64,128,256}"
OPS="${OPS:-Read,Write,Copy}"
EXTRA_FLAGS="${EXTRA_FLAGS:-}"

# Parse a byte size with an optional binary K/M/G suffix into plain bytes
# (afpd's "server quantum" parser rejects a trailing suffix).
to_bytes() {
    case "$1" in
        *[Kk]) echo $((${1%[Kk]} * 1024)) ;;
        *[Mm]) echo $((${1%[Mm]} * 1024 * 1024)) ;;
        *[Gg]) echo $((${1%[Gg]} * 1024 * 1024 * 1024)) ;;
        *[!0-9]*) fatal "QUANTUM must be an integer with an optional K/M/G suffix: $1" ;;
        *) echo "$1" ;;
    esac
}

SRV_IP=10.99.0.1
CLI_IP=10.99.0.2
NS=perfcli
IP=/sbin/ip
OUT=/output
NAME="${RUN_NAME:-${SUITE}-${RATE}}"
FRAG=/tmp/frags

fatal() {
    echo "FATAL: $*" >&2
    exit 1
}

cleanup() {
    "$IP" netns del "$NS" 2> /dev/null || true
    [ -n "${SRV_PID:-}" ] && kill "$SRV_PID" 2> /dev/null || true
}
trap cleanup EXIT INT TERM

[ -d "$OUT" ] || fatal "no /output mount; add -v <hostdir>:/output"
[ -e /proc/sys/net ] || fatal "no /proc/sys/net; run with --privileged"
"$IP" netns list > /dev/null 2>&1 || fatal "iproute2 netns unusable; run with --privileged"
[ "$SUITE" = speed ] || [ "$SUITE" = lan ] || fatal "SUITE must be speed or lan"

# Server-side afp.conf tuning, applied to the afpd started below via the
# testsuite entrypoint's afp.conf generation (empty => afpd defaults).
# QUANTUM accepts 1M, 512K, etc.; READAHEAD is a plain scale factor.
[ -n "${QUANTUM:-}" ] && export AFP_SERVER_QUANTUM="$(to_bytes "$QUANTUM")"
[ -n "${READAHEAD:-}" ] && export AFP_DSIREADBUF="$READAHEAD"

echo "=== netatalk perftest: suite=$SUITE rate=$RATE delay=$DELAY mtu=$MTU iter=$ITER quantum=${QUANTUM:-default} readahead=${READAHEAD:-default} ===" >&2

# --- afpd in the root namespace, standard testsuite configuration ---
TESTSUITE="" /entrypoint.sh > /dev/null 2>&1 &
SRV_PID=$!

i=0
while ! nc -z 127.0.0.1 548 2> /dev/null; do
    i=$((i + 1))
    [ "$i" -ge 30 ] && fatal "afpd did not come up within 30s"
    sleep 1
done

# --- client namespace joined by a shaped veth pair ---
"$IP" netns add "$NS"
"$IP" link add v-srv type veth peer name v-cli
"$IP" link set v-cli netns "$NS"
"$IP" addr add "$SRV_IP/24" dev v-srv
"$IP" link set v-srv up mtu "$MTU"
"$IP" netns exec "$NS" "$IP" addr add "$CLI_IP/24" dev v-cli
"$IP" netns exec "$NS" "$IP" link set v-cli up mtu "$MTU"
"$IP" netns exec "$NS" "$IP" link set lo up

# Disable segmentation offloads so the veth counters see on-wire sized
# packets, not 64 KB GSO aggregates that never get segmented on a
# virtual link.
ethtool -K v-srv tso off gso off gro off > /dev/null
"$IP" netns exec "$NS" ethtool -K v-cli tso off gso off gro off > /dev/null

tc qdisc add dev v-srv root netem rate "$RATE" delay "$DELAY"
"$IP" netns exec "$NS" tc qdisc add dev v-cli root netem rate "$RATE" delay "$DELAY"

i=0
while ! "$IP" netns exec "$NS" nc -z "$SRV_IP" 548 2> /dev/null; do
    i=$((i + 1))
    [ "$i" -ge 10 ] && fatal "afpd unreachable over the shaped veth"
    sleep 1
done

# Exact both-direction counters from the client veth, read before and
# after each test invocation while the link is idle.  Printed as awk
# does the arithmetic: the raw counters exceed 32-bit shell/awk int on
# long runs, so sums are done in awk with double precision.
ifstat() {
    for f in tx_packets tx_bytes rx_packets rx_bytes; do
        "$IP" netns exec "$NS" cat "/sys/class/net/v-cli/statistics/$f"
    done | tr '\n' ' '
}

# Capture the connection MSS while a test connection is live; keeps the
# first non-empty observation.
capture_mss() {
    if [ ! -s /tmp/mss ]; then
        "$IP" netns exec "$NS" ss -Hti "dst $SRV_IP" 2> /dev/null \
            | grep -oE 'mss:[0-9]+' | head -1 | cut -d: -f2 > /tmp/mss || true
    fi
}

run_tool() {
    # shellcheck disable=SC2086
    "$IP" netns exec "$NS" "$@" $EXTRA_FLAGS \
        -7 -h "$SRV_IP" -p 548 -u "$AFP_USER" -w "$AFP_PASS" \
        -s "$SHARE_NAME" -n "$ITER"
}

: > "$OUT/$NAME-stderr.log"
NETCSV="$OUT/$NAME-net.csv"
echo "size_mb,tx_packets,tx_bytes,rx_packets,rx_bytes,avg_tx_bytes,avg_rx_bytes" > "$NETCSV"

: > /tmp/mss

if [ "$SUITE" = lan ]; then
    before=$(ifstat)
    run_tool afp_lantest -c > "$OUT/$NAME.csv" 2>> "$OUT/$NAME-stderr.log" &
    TOOL_PID=$!
    sleep 2
    capture_mss
    wait "$TOOL_PID"
    after=$(ifstat)
    echo "all $before $after" | awk '{
        printf "all,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f\n",
            $6-$2, $7-$3, $8-$4, $9-$5,
            ($6-$2 > 0) ? ($7-$3)/($6-$2) : 0,
            ($8-$4 > 0) ? ($9-$5)/($8-$4) : 0}' >> "$NETCSV"
else
    # one invocation per sweep size: exact per-size network deltas
    rm -rf "$FRAG"
    mkdir -p "$FRAG"
    n=0
    for size in $(echo "$SWEEP" | tr ',' ' '); do
        n=$((n + 1))
        before=$(ifstat)
        run_tool afp_speedtest -c -z "$size" -f "$OPS" \
            > "$FRAG/$(printf '%02d' "$n").csv" 2>> "$OUT/$NAME-stderr.log" &
        TOOL_PID=$!
        sleep 1
        capture_mss
        wait "$TOOL_PID" || fatal "afp_speedtest failed at size $size (see $NAME-stderr.log)"
        after=$(ifstat)
        echo "$size $before $after" | awk '{
            printf "%s,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f\n", $1,
                $6-$2, $7-$3, $8-$4, $9-$5,
                ($6-$2 > 0) ? ($7-$3)/($6-$2) : 0,
                ($8-$4 > 0) ? ($9-$5)/($8-$4) : 0}' >> "$NETCSV"
    done

    # Merge the fragments into one CSV with the layout of a single sweep
    # run: one header, all per-iteration rows in sweep order, each op's
    # summary table assembled across fragments, the last TCP metrics
    # block.  POSIX awk only (no ENDFILE); FNR==1 resets section state,
    # first-seen order of ops is preserved.
    awk '
        FNR == 1 { frag++; insum = 0; intcp = 0 }
        /^# File Size Performance Summary/ { insum = 1 }
        /^# TCP Metrics Evolution Summary/ { intcp = 1 }
        !insum && !intcp {
            if ($0 ~ /^test_name,/) {
                if (frag == 1) rows = rows $0 "\n"
                next
            }
            if ($0 != "") rows = rows $0 "\n"
            next
        }
        insum && !intcp {
            if ($0 ~ /^# File Size Performance Summary for /) {
                op = $NF
                if (!(op in seen)) { seen[op] = 1; order[++nops] = op }
                next
            }
            if ($0 ~ /^file_size_bytes,/) { hdr[op] = $0; next }
            if ($0 != "") sums[op] = sums[op] $0 "\n"
            next
        }
        intcp {
            if ($0 ~ /^# TCP Metrics Evolution Summary/) tcp = ""
            tcp = tcp $0 "\n"
        }
        END {
            printf "%s", rows
            for (i = 1; i <= nops; i++) {
                op = order[i]
                print ""
                print "# File Size Performance Summary for " op
                print hdr[op]
                printf "%s", sums[op]
            }
            print ""
            printf "%s", tcp
        }
    ' "$FRAG"/*.csv > "$OUT/$NAME.csv"
fi

# --- run summary ---
mss=$(cat /tmp/mss 2> /dev/null)
{
    echo "suite: $SUITE"
    echo "rate: $RATE"
    echo "delay: $DELAY"
    echo "mtu: $MTU"
    echo "iterations: $ITER"
    echo "quantum: ${QUANTUM:-default}"
    echo "readahead: ${READAHEAD:-default}"
    echo "mss: ${mss:-unknown}"
    awk -F, 'NR > 1 {tp += $2; tb += $3; rp += $4; rb += $5}
             END {
                 printf "tx_packets_total: %.0f\ntx_bytes_total: %.0f\n", tp, tb
                 printf "rx_packets_total: %.0f\nrx_bytes_total: %.0f\n", rp, rb
                 if (tp > 0) printf "avg_tx_bytes: %.0f\n", tb / tp
                 if (rp > 0) printf "avg_rx_bytes: %.0f\n", rb / rp
             }' "$NETCSV"
} > "$OUT/$NAME-net.txt"

echo "=== perftest done: $NAME.csv, $NAME-stderr.log, $NAME-net.csv, $NAME-net.txt in /output ===" >&2
