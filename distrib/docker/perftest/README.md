# Netatalk performance test suite

Measures AFP transfer performance over a rate-shaped virtual network and
reports throughput plus TCP segment accounting, so wire-level effects
(segment counts, MSS interaction with the DSI server quantum) can be
verified rather than inferred from loopback runs.

## How it works

The perftest image is the standard testsuite image
(`distrib/docker/testsuite_alp.Dockerfile`) plus iproute2/tc. At run
time, inside one privileged container:

1. afpd starts in the container's root network namespace with the usual
   testsuite configuration.
2. A client network namespace is created, joined to the root namespace
   by a veth pair.
3. Both veth ends get a `tc netem` qdisc applying the configured rate
   and one-way delay.
4. For `SUITE=speed`, every sweep size runs as its own `afp_speedtest`
   invocation; the client veth's packet and byte counters (both
   directions) are read before and after each invocation while the link
   is idle, so per-size network statistics are exact, not sampled.
5. The per-size CSV fragments are merged into one CSV with the same
   layout as a single CI sweep run; network statistics go to separate
   files and never enter the CSV the plot scripts consume.

Network namespaces, veth and tc are Linux kernel features: on macOS or
Windows the rig runs unchanged inside Docker Desktop's Linux VM.
Throughput numbers include VM overhead; segment counts are exact.

## Usage

From the repository root:

Every option is a `KEY=value` argument or an environment variable; a
command-line argument overrides the environment.

```sh
# default: speedtest, 10gbit, 2ms one-way delay, MTU 1500, CI sweep
distrib/docker/perftest/run_perftest.sh

# latency sweep — one CSV, plot and net stats per delay
distrib/docker/perftest/run_perftest.sh DELAY=0.5ms,1ms,1.5ms,2ms

# rate sweep
distrib/docker/perftest/run_perftest.sh RATE=100mbit,1gbit,10gbit

# jumbo frames, lantest instead of speedtest (args and env are equivalent)
distrib/docker/perftest/run_perftest.sh MTU=9000 SUITE=lan RATE=10gbit
MTU=9000 SUITE=lan RATE=10gbit distrib/docker/perftest/run_perftest.sh

# larger server quantum, lower latency
distrib/docker/perftest/run_perftest.sh QUANTUM=2M DELAY=1ms
```

Run `run_perftest.sh --help` for the full option list.

| Option        | Default            | Meaning                                  |
| ------------- | ------------------ | ---------------------------------------- |
| `SUITE`       | `speed`            | `speed` or `lan`                         |
| `RATE`        | `10gbit`           | tc netem rate; comma-separated to sweep rates |
| `DELAY`       | `2ms`              | one-way delay (RTT is twice this); comma-separated to sweep latencies |
| `MTU`         | `1500`             | veth MTU (`9000` for jumbo)              |
| `ITER`        | `5`                | iterations, as CI                        |
| `SWEEP`       | CI sweep list      | speedtest `-z` size list (MB)            |
| `OPS`         | `Read,Write,Copy`  | speedtest operations (see note)          |
| `QUANTUM`     | afpd default (1M)  | server quantum: bytes or K/M/G, e.g. `1M`, `512K` |
| `READAHEAD`   | afpd default       | dsireadbuf scale factor (plain integer)  |
| `EXTRA_FLAGS` | empty              | extra flags for the test program         |
| `OUTDIR`      | `perftest-results` | host output directory                    |

ServerCopy is excluded from the default operations, unlike CI: it is
`FPCopyFile`, the server copies the file to itself and no file data
crosses the shaped link, so its curve reflects server-local disk speed
and visually dwarfs the network-bound operations. Set
`OPS=Read,Write,Copy,ServerCopy` for the exact CI operation set.

`RATE` and `DELAY` each accept a comma-separated list; every rate x
delay combination runs in a fresh container, and a swept axis is added
to the output basename (e.g. `speed-10gbit-1ms`) so runs never
overwrite each other. To run the entrypoint against an already-built
image, see the `docker run` invocation inside `run_perftest.sh`.

## Interpreting the output

- The merged CSV is format-identical to the CI perfgraph artifact and
  renders with the same `contrib/scripts/plot_*.pl` scripts.
- `<run>-net.csv` holds the exact per-size interface counters:
  tx/rx packets and bytes over the client veth for that size's
  invocation, plus average bytes per packet in each direction.  The
  per-size table printed at the end makes runt effects visible where
  they matter — small and mid sizes — instead of being averaged away by
  the large transfers.
- Average packet size approaching the MTU-limited maximum means
  well-filled frames; a drop against the same transfer size indicates
  more small segments (runts or command round-trips).  Interface
  counters include TCP/IP headers and ACK-only packets, so compare
  between runs, not against the raw MSS.
- `mss:` in the connection sample is the effective MSS on the shaped
  path (1448 at MTU 1500 with TCP timestamps).
- The namespace OutSegs delta includes ACKs and control segments; use
  the per-connection `segs_out` for frame-level comparisons between
  runs — identical transfers at different rates should acknowledge the
  same bytes while segment counts vary only with TCP coalescing.

## Files

- `perftest.Dockerfile` — testsuite image + iproute2/tc, perftest entrypoint
- `entrypoint_perftest.sh` — the in-container rig (namespace, shaping, runs)
- `run_perftest.sh` — host-side build-and-run wrapper
