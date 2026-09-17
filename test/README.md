# Tests

This directory contains two complementary test modules for `afpd`.

## afpd

`afpd/` contains the server-side integration tests.  Its `afpdtest` harness
links against the `afpd` implementation and exercises internal server paths
directly, using a temporary SQLite-backed test configuration and volumes
created by `test.sh`.  The harness emits Test Anything Protocol (TAP) output;
Meson consumes it as a TAP test when configured with `-Dwith-tests=true`.

Put a test here when the behaviour is specific to an `afpd` implementation
detail or error path that is difficult or impossible to control through the
AFP wire protocol.  Typical examples include configuration loading, CNID and
directory-cache state, descriptor handling, cleanup, and injected system-call
failures.  Add the test implementation and its declaration/registration to
the appropriate `subtests*` or `test*` source files; update
`afpd/meson.build` when a new source file is needed.

The harness owns its temporary test volumes and configuration.  Tests must
clean up any state they create so they remain independent and repeatable.

`afpd/` also contains `afpd_fuzz`, a libFuzzer target for AFP file and
directory operations.  Enable it with `-Dwith-fuzzing=true`; see
[`afpd/FUZZING.md`](afpd/FUZZING.md) for build and usage instructions.

### Code coverage

Build the afpd tests with GCC or Apple Clang coverage instrumentation, then use gcovr to
collect gcov data and report line, function, and branch coverage. The commands
below run both directory-cache modes and measure `etc/afpd` and `libatalk`,
excluding test harness and third-party sources.

#### GCC on Linux

On Linux, install GCC (including G++ and the matching gcov), gcovr, Meson,
Ninja, and the usual build dependencies, including SQLite development headers.
From the repository root, configure a fresh build directory and run the tests:

```sh
CC=gcc CXX=g++ meson setup build-coverage \
    -Dbuildtype=debug \
    -Db_coverage=true \
    -Dwith-appletalk=true \
    -Dwith-cnid-backends=sqlite \
    -Dwith-docs= \
    -Dwith-dtrace=false \
    -Dwith-init-style=none \
    -Dwith-tests=true
meson compile -C build-coverage
meson test -C build-coverage 'afpd tests*' --print-errorlogs
```

Generate the coverage reports:

```sh
mkdir -p build-coverage/coverage
gcovr --root . --gcov-executable gcov \
    --filter 'etc/afpd/' --filter 'libatalk/' \
    --html-details build-coverage/coverage/index.html \
    --xml-pretty --xml build-coverage/coverage/coverage.xml \
    --txt build-coverage/coverage/coverage.txt \
    --print-summary build-coverage
```

The command prints coverage totals to the terminal. Open
`build-coverage/coverage/index.html` in a browser for annotated source coverage.
The same directory contains `coverage.xml` in Cobertura format and
`coverage.txt` with per-file statistics. Test logs are in
`build-coverage/meson-logs/`.

#### Apple Clang on macOS

Install Xcode or the Command Line Tools, plus gcovr, Meson, Ninja, and the
usual build dependencies through Homebrew. From the repository root, use a
separate build directory for Apple Clang:

```sh
CC=clang CXX=clang++ \
    meson setup build-coverage \
    -Dbuildtype=debug \
    -Db_coverage=true \
    -Dwith-homebrew=true \
    -Dwith-cnid-backends=sqlite \
    -Dwith-docs= \
    -Dwith-dtrace=false \
    -Dwith-init-style=none \
    -Dwith-tests=true
meson compile -C build-coverage
meson test -C build-coverage 'afpd tests*' --print-errorlogs
mkdir -p build-coverage/coverage
gcovr --root . --gcov-executable 'xcrun llvm-cov gcov' \
    --filter 'etc/afpd/' --filter 'libatalk/' \
    --html-details build-coverage/coverage/index.html \
    --xml-pretty --xml build-coverage/coverage/coverage.xml \
    --txt build-coverage/coverage/coverage.txt \
    --print-summary build-coverage
```

Apple Clang produces gcov-compatible coverage data, which gcovr reads through
LLVM's gcov reader supplied by Xcode. Fault-injection tests that require
`LD_PRELOAD` interposition skip when it is unavailable on macOS, so coverage
totals will differ from Linux.

#### Interpreting results

Coverage describes the code compiled for this configuration and exercised by
the harness. Optional features and platform-dependent test skips affect the
results; server startup and other paths that the harness bypasses remain
uncovered. Repeated runs accumulate coverage counters, so use a fresh build
directory when measuring a new baseline.

## testsuite

`testsuite/` contains AFP client programs that test a running AFP server over
DSI/AFP.  The main program, `afp_spectest`, provides AFP command and
specification-compliance coverage; `afp_logintest` covers DSI sessions and
authentication.  The directory also contains client tools such as `afparg`,
benchmarks, and their shared client/test-reporting code.  Build it with
`-Dwith-testsuite=true`; unlike `afpd/`, these programs are run against a
server and volume supplied by the test user or environment.

Put a test here when it verifies behaviour visible to an AFP client: request
and reply semantics, protocol-version compatibility, authentication, access
control, locking, filesystem operations, or interoperability.  Add AFP
command coverage to the matching `FP*.c` or `T2_*.c` testset.  `FP*.c`
(tier 1) tests use only the AFP wire protocol; `T2_*.c` (tier 2) tests also
require the test runner to have direct access to the host filesystem backing
the test volume.  Register a new source file in both `spectest.c` and
`testsuite/meson.build`.  Add session or UAM coverage to the
`logintest*`/`afptest_uam*` code instead.

## Choosing a module

Use `testsuite/` by default for a user-visible AFP correctness test.
Use `afpd/` when the test depends on server-private state, a controlled fault,
or a path that cannot be reached reliably by an external AFP client.
A change may warrant one test in each module: an `afpd/` test for a precise
internal edge case and a `testsuite/` test to protect the client-observable
contract.
