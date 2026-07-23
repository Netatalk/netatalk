#!/bin/sh
# Build and run the netatalk performance test suite over shaped links.
#
# Builds the testsuite image from the working tree, layers the perftest
# tooling on top, runs the CI perfgraph speedtest (or lantest) once per
# requested link rate, renders the same plots the CI runners publish, and
# prints a network statistics comparison table across rates.  Requires
# Docker with a Linux backend; plot rendering needs perl and gnuplot on
# the host (brew install gnuplot / apt install gnuplot-nox).
#
# Usage:
#   distrib/docker/perftest/run_perftest.sh [KEY=value ...]
#
# Every option may be given as a KEY=value argument or an environment
# variable; a command-line argument overrides the environment.  Options:
#
#   SUITE        speed | lan                 (default: speed)
#   RATE         tc rate, comma-separated to sweep rates
#                (default: 10gbit; e.g. RATE=100mbit,1gbit,10gbit)
#   DELAY        one-way delay, comma-separated to sweep latencies
#                (default: 2ms, RTT is twice this; e.g. DELAY=0.5ms,1ms,2ms)
#   MTU          veth MTU                    (default: 1500; 9000 for jumbo)
#   ITER         iterations                  (default: 5)
#   SWEEP        speedtest -z size list      (default: the CI perfgraph sweep)
#   OPS          speedtest operations        (default: Read,Write,Copy)
#   QUANTUM      server quantum, bytes or K/M/G suffix e.g. 1M, 512K
#   READAHEAD    dsireadbuf scale factor
#   EXTRA_FLAGS  extra flags for the test program
#   OUTDIR       host output directory       (default: perftest-results)
#
# Example:
#   run_perftest.sh RATE=1gbit,10gbit DELAY=1ms QUANTUM=2M SUITE=speed
#
# Results land in $OUTDIR under the repository root.

set -eu

usage() {
    cat << 'EOF'
Build and run the netatalk performance test suite over shaped links.

Usage:
  run_perftest.sh [KEY=value ...]

Every option may be given as a KEY=value argument or an environment
variable; a command-line argument overrides the environment.

  SUITE        speed | lan                 (default: speed)
  RATE         tc rate, comma-separated to sweep rates
               (default: 10gbit; e.g. RATE=100mbit,1gbit,10gbit)
  DELAY        one-way delay, comma-separated to sweep latencies
               (default: 2ms, RTT is twice this; e.g. DELAY=0.5ms,1ms,2ms)
  MTU          veth MTU                    (default: 1500; 9000 for jumbo)
  ITER         iterations                  (default: 5)
  SWEEP        speedtest -z size list      (default: the CI perfgraph sweep)
  OPS          speedtest operations        (default: Read,Write,Copy)
  QUANTUM      server quantum, bytes or K/M/G suffix e.g. 1M, 512K
  READAHEAD    dsireadbuf scale factor
  EXTRA_FLAGS  extra flags for the test program
  OUTDIR       host output directory       (default: perftest-results)

RATE and DELAY may each be a comma-separated list; every rate x delay
combination runs.  A swept axis is encoded into the output basename so
runs never overwrite each other.

Example:
  run_perftest.sh DELAY=0.5ms,1ms,1.5ms,2ms QUANTUM=2M
EOF
}

# Defaults, overridable by the environment then by KEY=value arguments.
SUITE="${SUITE:-speed}"
RATE="${RATE:-10gbit}"
DELAY="${DELAY:-2ms}"
MTU="${MTU:-1500}"
ITER="${ITER:-5}"
OUTDIR="${OUTDIR:-perftest-results}"
SWEEP="${SWEEP:-}"
OPS="${OPS:-}"
QUANTUM="${QUANTUM:-}"
READAHEAD="${READAHEAD:-}"
EXTRA_FLAGS="${EXTRA_FLAGS:-}"

while [ $# -gt 0 ]; do
    case "$1" in
        -h | --help | help)
            usage
            exit 0
            ;;
        SUITE=*) SUITE=${1#*=} ;;
        RATE=*) RATE=${1#*=} ;;
        DELAY=*) DELAY=${1#*=} ;;
        MTU=*) MTU=${1#*=} ;;
        ITER=*) ITER=${1#*=} ;;
        SWEEP=*) SWEEP=${1#*=} ;;
        OPS=*) OPS=${1#*=} ;;
        QUANTUM=*) QUANTUM=${1#*=} ;;
        READAHEAD=*) READAHEAD=${1#*=} ;;
        EXTRA_FLAGS=*) EXTRA_FLAGS=${1#*=} ;;
        OUTDIR=*) OUTDIR=${1#*=} ;;
        *)
            echo "unknown argument: $1" >&2
            echo "run with --help for the option list" >&2
            exit 1
            ;;
    esac
    shift
done

cd "$(dirname "$0")/../../.."

TESTSUITE_TAG=netatalk-testsuite
PERFTEST_TAG=netatalk-perftest

# Chart quantum label: QUANTUM (bytes or K/M/G) in KiB; afpd's 1 MiB
# default = 1024 KiB when unset.
case "$QUANTUM" in
    '') QUANTUM_KB=1024 ;;
    *[Kk]) QUANTUM_KB=${QUANTUM%[Kk]} ;;
    *[Mm]) QUANTUM_KB=$((${QUANTUM%[Mm]} * 1024)) ;;
    *[Gg]) QUANTUM_KB=$((${QUANTUM%[Gg]} * 1024 * 1024)) ;;
    *) QUANTUM_KB=$((QUANTUM / 1024)) ;;
esac

mkdir -p "$OUTDIR"

echo "=== building $TESTSUITE_TAG from the working tree ==="
docker build -q -f distrib/docker/testsuite_alp.Dockerfile -t "$TESTSUITE_TAG" .

echo "=== building $PERFTEST_TAG ==="
docker build -q -f distrib/docker/perftest/perftest.Dockerfile \
    --build-arg BASE_IMAGE="$TESTSUITE_TAG" -t "$PERFTEST_TAG" .

RATES=$(echo "$RATE" | tr ',' ' ')
DELAYS=$(echo "$DELAY" | tr ',' ' ')

# A single-value delay axis stays out of the run name; rate always leads
# the name, so multiple rates never collide.
n_delays=$(
    set -- $DELAYS
    echo $#
)

# Names of every run performed, for the summary/stats tables below.
NAMES=""

for rate in $RATES; do
    for delay in $DELAYS; do
        # Encode a swept axis in the name so runs never overwrite; a
        # single-value axis is omitted for a stable, tidy basename.
        name="$SUITE-$rate"
        [ "$n_delays" -gt 1 ] && name="$name-$delay"
        NAMES="$NAMES $name"

        echo ""
        echo "################ suite: $SUITE  rate: $rate  delay: $delay ################"
        docker run --rm --privileged \
            --env AFP_USER=atalk1 --env AFP_PASS=test \
            --env SHARE_NAME=test1 --env AFP_GROUP=afpusers \
            --env TZ=UTC --env INSECURE_AUTH=1 \
            --env SUITE="$SUITE" \
            --env RATE="$rate" \
            --env DELAY="$delay" \
            --env MTU="$MTU" \
            --env ITER="$ITER" \
            --env RUN_NAME="$name" \
            ${SWEEP:+--env SWEEP="$SWEEP"} \
            ${OPS:+--env OPS="$OPS"} \
            ${QUANTUM:+--env QUANTUM="$QUANTUM"} \
            ${READAHEAD:+--env READAHEAD="$READAHEAD"} \
            ${EXTRA_FLAGS:+--env EXTRA_FLAGS="$EXTRA_FLAGS"} \
            -v "$PWD/$OUTDIR:/output" \
            "$PERFTEST_TAG"

        # render with the same script and options the CI perfgraph jobs
        # use; only title/subtitle mention the local link shaping
        if command -v gnuplot > /dev/null 2>&1; then
            if [ "$SUITE" = speed ]; then
                perl contrib/scripts/plot_speedtest.pl "$OUTDIR/$name.csv" \
                    -o "$OUTDIR/$name" \
                    --title "Netatalk AFP Speedtest — throughput vs file size" \
                    --subtitle "local perftest · rate $rate, delay $delay, mtu $MTU · mean MB/s, shaded band = min–max" \
                    --meta afp=3.4 \
                    --meta quantum_kb="$QUANTUM_KB" \
                    --meta iterations="$ITER" \
                    --meta warmup=1 \
                    --meta rate="$rate"
            else
                perl contrib/scripts/plot_lantest.pl "$OUTDIR/$name.csv" \
                    -o "$OUTDIR/$name" \
                    --title "Netatalk AFP Lantest — per-test latency" \
                    --subtitle "local perftest · rate $rate, delay $delay, mtu $MTU · candle: min–max wick, mean±σ box, median line" \
                    --meta afp=3.4 \
                    --meta iterations="$ITER" \
                    --meta quantum_kb="$QUANTUM_KB" \
                    --meta rate="$rate"
            fi
        else
            echo "NOTE: gnuplot not found on host; skipping plot for $name" >&2
        fi
    done
done

# --- comparison table across all rates in this invocation ---
echo ""
echo "================ perftest summary ================"
printf '%-22s %-6s %-6s %-12s %-12s %-8s %-8s %s\n' \
    "run" "mss" "mtu" "tx-packets" "rx-packets" "avg-tx" "avg-rx" "plot"
for name in $NAMES; do
    net="$OUTDIR/$name-net.txt"
    [ -f "$net" ] || continue
    mss=$(awk '/^mss:/ {print $2}' "$net")
    mtu=$(awk '/^mtu:/ {print $2}' "$net")
    txp=$(awk '/^tx_packets_total:/ {print $2}' "$net")
    rxp=$(awk '/^rx_packets_total:/ {print $2}' "$net")
    avgtx=$(awk '/^avg_tx_bytes:/ {print $2}' "$net")
    avgrx=$(awk '/^avg_rx_bytes:/ {print $2}' "$net")
    plot="$OUTDIR/$name.png"
    [ -f "$plot" ] || plot="(not rendered)"
    printf '%-22s %-6s %-6s %-12s %-12s %-8s %-8s %s\n' \
        "$name" "$mss" "$mtu" "$txp" "$rxp" "$avgtx" "$avgrx" "$plot"
done

echo ""
echo "================ per-size network statistics ================"
for name in $NAMES; do
    netcsv="$OUTDIR/$name-net.csv"
    [ -f "$netcsv" ] || continue
    echo ""
    echo "--- $name (client veth: tx = client sends, rx = client receives) ---"
    awk -F, 'NR == 1 {
                 printf "%-10s %-12s %-14s %-12s %-14s %-8s %-8s\n",
                     "size_mb", $2, $3, $4, $5, "avg-tx", "avg-rx"
                 next
             }
             {
                 printf "%-10s %-12s %-14s %-12s %-14s %-8s %-8s\n",
                     $1, $2, $3, $4, $5, $6, $7
             }' "$netcsv"
done
echo ""
echo "Raw per-size data: $OUTDIR/<run>-net.csv"
