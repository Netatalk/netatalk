# Performance Analysis and Profiling

Performance work starts with a repeatable workload and the state it exercises.
Separate transfer throughput, directory traversal, metadata, search, and
contention before changing buffers or caches. Configuration syntax and
defaults belong in [afp.conf(5)](https://netatalk.io/manual/en/afp.conf.5.html);
this chapter explains how to assess their implementation effects.

## Workloads and measurements

Record the source revision, Meson options, client version, execution mode,
storage format, database/search backend, configuration, and session count.
Then compare cold and warm runs of the same workload, changing one relevant
control at a time. Check correctness and stale-result behavior as well as
speed, especially with external writers.

| Workload | Measurements and likely boundaries |
| --- | --- |
| Bulk reads/writes | Throughput, CPU, blocked time, socket behavior, eligible sendfile/recvfile paths |
| Directory enumeration | Latency, stat work, metadata reads, cache/ghost hits, and filesystem I/O |
| Small-file creation or rename | Latency, database write contention, metadata updates, and invalidation |
| Search | Backend/index coverage, query latency, candidate/result limits, and permission filtering |
| Concurrent or retained sessions | Per-worker memory, open forks/descriptors, database contention, and hint drops |

Build the AFP client tools with `-Dwith-testsuite=true`. `lantest` exercises
AFP workloads; `speedtest` and `speedtest_local_vfs` help compare transfer
and local VFS costs. Use their current usage and `test/README.md` for
invocation and environment requirements.

## Validation cost and storage pressure

A cache hit can still perform validation. Repeatedly checking thousands of
cached objects creates stat work and can pressure filesystem metadata
caches and storage even when little file data is transferred. Parent-path
resolution adds work when a cached ancestor is unavailable. Measure stat
activity, CPU, I/O, and cache behavior together rather than equating every
stat call with a physical disk read.

Sampled validation reduces that work by reusing cached information between
checks. Its benefit and stale-state cost depend on access patterns and
external writers. A configured frequency is not a guarantee that a whole
directory enumeration will avoid stat calls; each parameter path can have
additional filesystem requirements.

Compare cache hit/miss and eviction counts, `invalid_on_use`, parent-path
repairs, and hint drops alongside latency. ARC ghosts consume full entry
objects in this implementation, and resource-fork content has its own byte
budget. An improved hit ratio alone does not establish reduced memory or
storage demand. See [Caching and Consistency](caching.md) for counter meanings.

## Buffer and memory accounting

DSI's server quantum limits incoming write payloads. Receive readahead is
the quantum multiplied by `dsireadbuf`; it does not prefetch file data from
disk. With the configured 1 MiB quantum and multiplier 32, the command and
readahead buffers account for approximately 33 MiB per worker before session
alignment and other state. This is an allocation estimate, not total RSS.

Add reply buffers, live and ghost directory objects, variable-length names,
resource-fork buffers actually populated, database state, open forks, thread
stacks, and kernel socket/pipe memory. The configured resource budget is a
limit rather than an allocation made in full at startup. Multiply measured
worker memory by the relevant active, sleeping, and disconnected sessions.

The parser caps the readahead product at 1 GiB with address-space checks,
and session quantum alignment re-clamps it. Kernel socket buffers and
recvfile pipe capacity can differ from requested values; inspect the actual
values logged by the transport. Link speed and filesystem record size alone
do not establish an appropriate quantum or buffer requirement.

Sendfile/recvfile eligibility depends on build, platform, and fork storage.
Compare enabled and disabled runs that actually reach the relevant path.
Similarly, compare database contention and query latency before choosing a
backend; SQLite WAL and an external MySQL connection are different workloads,
not a universal speed ranking. Search backends also have different indexing
and query costs; see [Indexed Spotlight Search](indexing.md).

## Flamegraph workflow

Netatalk provides a containerized profiling workflow that generates
interactive SVG flamegraphs of the **afpd** daemon while running
the AFP test suite. This is useful for identifying CPU hotspots,
verifying performance optimizations, and understanding code paths
exercised by specific test workloads.

The profiling infrastructure uses Linux **perf** for sampling and
Brendan Gregg's [FlameGraph](https://github.com/brendangregg/FlameGraph)
tools to produce the visualization.

### Prerequisites

- Docker (or Podman) with the ability to run `--privileged` containers
- A clone of the Netatalk source tree

### Quick Start

Build the debug profiling container:

```shell
docker build --no-cache -t netatalk_debug \
    -f distrib/docker/debug_alp.Dockerfile .
```

Run the spectest with on-CPU flamegraph profiling:

```shell
docker run --rm --privileged --network host \
    --volume "/var/run/dbus:/var/run/dbus" \
    --env AFP_USER=atalk1 \
    --env AFP_PASS=test \
    --env AFP_USER2=atalk2 \
    --env AFP_PASS2=test \
    --env SHARE_NAME=test1 \
    --env SHARE_NAME2=test2 \
    --env AFP_VERSION=7 \
    --env AFP_GROUP=afpusers \
    --env TZ=Europe/London \
    --env TESTSUITE=spec \
    --env INSECURE_AUTH=1 \
    --env DISABLE_TIMEMACHINE=1 \
    --env FLAMEGRAPH=1 \
    netatalk_debug > flamegraph.svg
```

Open `flamegraph.svg` in a web browser. The interactive SVG allows
clicking to zoom into specific call stacks and hovering to see
sample counts.

### How It Works

The debug Dockerfile builds Netatalk with:

- **-Dbuildtype=debugoptimized** — compiler flags `-O2 -g`, giving
  realistic performance while preserving debug symbols for stack
  frame resolution.
- **-Dc_args=-fno-omit-frame-pointer** — ensures GCC preserves
  frame pointers so `perf` can accurately unwind call stacks.

The debug entrypoint script wraps the normal test execution with
profiling:

1. Starts **netatalk** (which spawns **afpd**)
2. Finds afpd PIDs and starts `perf record` in the background
3. Runs the selected test suite
4. Stops `perf record` and generates the flamegraph SVG
5. Emits the SVG to stdout (all other output goes to stderr)

The fd-redirect approach (`stdout=SVG`, `stderr=logs`) means you
capture the SVG cleanly with shell redirection while still seeing
test progress on the terminal.

## Profiling Modes

### On-CPU (default)

Samples the CPU at a fixed frequency to show where afpd spends
its **processing time**. Best for workloads that exercise complex
server-side logic.

```shell
--env FLAMEGRAPH=1
```

Recommended test suites: **spectest**, **lantest**

### Off-CPU

Traces kernel scheduler context switches to show where afpd
spends time **sleeping or blocked** (I/O waits, poll, locks).
The resulting flamegraph uses a blue color palette to visually
distinguish it from on-CPU flamegraphs.

```shell
--env FLAMEGRAPH=1
--env FLAMEGRAPH_OFFCPU=1
```

Recommended test suites: **speedtest** and other I/O-bound workloads

## Environment Variables

The following environment variables control the profiling behavior.
They are used in addition to the standard container environment
variables documented in `CONTAINERS.md`.

| Variable            | Description                                       |
|---------------------|---------------------------------------------------|
| FLAMEGRAPH          | Set to any value to enable profiling               |
| FLAMEGRAPH\_OFFCPU  | Set to any value to use off-CPU mode               |
| PERF\_FREQ          | On-CPU sampling frequency in Hz (default: 999)     |

## Test Suite Recommendations

### spectest

Produces the richest flamegraphs. The spectest exercises hundreds
of distinct AFP protocol operations including directory traversal,
file creation, metadata manipulation, authentication, volume
enumeration, and resource fork handling. The resulting flamegraph
shows a broad cross-section of afpd code paths.

### lantest

Exercises read/write/copy operations with protocol-level
verification. Produces good flamegraphs at the default 999 Hz
sampling rate showing the AFP command dispatch, DSI framing,
and file I/O paths.

### speedtest

Transfer workloads can spend much of their time waiting in `poll()` or I/O,
producing sparse on-CPU profiles. Compare **off-CPU mode**
(`FLAMEGRAPH_OFFCPU=1`) to locate waits, and record whether the client is
local or remote. Loopback results do not establish behavior under real
network latency or storage load.

## Files

| File                                          | Purpose                           |
|-----------------------------------------------|-----------------------------------|
| distrib/docker/debug\_alp.Dockerfile          | Alpine debug build with perf      |
| distrib/docker/debug\_entrypoint\_netatalk.sh | Entrypoint with profiling support |

## Interpreting Flamegraphs

- The **x-axis** represents the proportion of samples (wider = more time)
- The **y-axis** represents the call stack depth (bottom = entry point)
- **Color** is random for on-CPU (warm tones) and blue for off-CPU
- Click any frame to **zoom in**; click "Reset Zoom" to return
- Hover over a frame to see the **function name** and **sample count**

Look for wide frames near the top of the stacks — these are the
functions where afpd spends the most CPU time. Narrow frames deep
in the stack show the call path that led there.

See Brendan Gregg's
[Flame Graphs](https://www.brendangregg.com/flamegraphs.html)
page for a comprehensive guide to reading and interpreting
flamegraphs.
