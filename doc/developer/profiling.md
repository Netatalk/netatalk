Flamegraph Profiling
====================

Netatalk provides a containerized profiling workflow that generates
interactive SVG flamegraphs of the **afpd** daemon while running
the AFP test suite. This is useful for identifying CPU hotspots,
verifying performance optimizations, and understanding code paths
exercised by specific test workloads.

The profiling infrastructure uses Linux **perf** for sampling and
Brendan Gregg's [FlameGraph](https://github.com/brendangregg/FlameGraph)
tools to produce the visualization.

Prerequisites
-------------

- Docker (or Podman) with the ability to run `--privileged` containers
- A clone of the Netatalk source tree

Quick Start
-----------

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
    --env TESTSUITE=spectest \
    --env INSECURE_AUTH=1 \
    --env DISABLE_TIMEMACHINE=1 \
    --env FLAMEGRAPH=1 \
    netatalk_debug > flamegraph.svg
```

Open `flamegraph.svg` in a web browser. The interactive SVG allows
clicking to zoom into specific call stacks and hovering to see
sample counts.

How It Works
------------

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

Profiling Modes
---------------

On-CPU (default)
----------------

Samples the CPU at a fixed frequency to show where afpd spends
its **processing time**. Best for workloads that exercise complex
server-side logic.

```shell
--env FLAMEGRAPH=1
```

Recommended test suites: **spectest**, **lantest**

Off-CPU
-------

Traces kernel scheduler context switches to show where afpd
spends time **sleeping or blocked** (I/O waits, poll, locks).
The resulting flamegraph uses a blue color palette to visually
distinguish it from on-CPU flamegraphs.

```shell
--env FLAMEGRAPH=1
--env FLAMEGRAPH_OFFCPU=1
```

Recommended test suites: **speedtest** and other I/O-bound workloads

Environment Variables
---------------------

The following environment variables control the profiling behavior.
They are used in addition to the standard container environment
variables documented in `distrib/docker/CONTAINERS.md`.

| Variable            | Description                                       |
|---------------------|---------------------------------------------------|
| FLAMEGRAPH          | Set to any value to enable profiling               |
| FLAMEGRAPH\_OFFCPU  | Set to any value to use off-CPU mode               |
| PERF\_FREQ          | On-CPU sampling frequency in Hz (default: 999)     |

Test Suite Recommendations
--------------------------

spectest
--------

Produces the richest flamegraphs. The spectest exercises hundreds
of distinct AFP protocol operations including directory traversal,
file creation, metadata manipulation, authentication, volume
enumeration, and resource fork handling. The resulting flamegraph
shows a broad cross-section of afpd code paths.

lantest
-------

Exercises read/write/copy operations with protocol-level
verification. Produces good flamegraphs at the default 999 Hz
sampling rate showing the AFP command dispatch, DSI framing,
and file I/O paths.

speedtest
---------

Over loopback, the speedtest is I/O-bound — afpd spends most of
its wall time sleeping in `poll()` waiting for the next request.
On-CPU flamegraphs show minimal data because the DSI layer
processes each request in microseconds. Use **off-CPU mode**
(`FLAMEGRAPH_OFFCPU=1`) to see where afpd blocks, or run against
a remote host with real network latency for meaningful on-CPU data.

Files
-----

| File                                          | Purpose                           |
|-----------------------------------------------|-----------------------------------|
| distrib/docker/debug\_alp.Dockerfile          | Alpine debug build with perf      |
| distrib/docker/debug\_entrypoint\_netatalk.sh | Entrypoint with profiling support |

Interpreting Flamegraphs
------------------------

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
