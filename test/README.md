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
