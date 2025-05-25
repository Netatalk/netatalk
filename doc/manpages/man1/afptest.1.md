# Name

afp_lantest, afp_logintest, afp_spectest, afp_speedtest, afparg, fce_listen — AFP protocol tests

# Synopsis

**afp_lantest** [-34567GgVv] [-h *host*] [-p *port*] [-s *volume*] [-u *user*] [-w *password*] [-n *iterations*]
[-t *tests*] [-F *bigfile*]

**afp_logintest** [-1234567CmVv] [-h *host*] [-p *port*] [-s *volume*] [-u *user*] [-w *password*]

**afp_spectest** [-1234567aCiLlmVvXx] [-h *host*] [-H *host2*] [-p *port*] [-s *volume*] [-c *path to volume*]
[-S *volume2*] [-u *user*] [-d *user2*] [-w *password*] [-f *test*]

**afp_speedtest** [-1234567aeiLnVvy] [-h *host*] [-p *port*] [-s *volume*] [-S *volume2*] [-u *user*]
[-w *password*] [-n *iterations*] [-d *size*] [-q *quantum*] [-F *file*] [-f *test*]

**afparg** [-1234567lVv] [-h *host*] [-p *port*] [-s *volume*] [-u *user*] [-w *password*] [-f *command*]

**fce_listen** [-h *host*] [-p *port*]

# Description

The AFP testsuite contains several utilities aimed at testing AFP servers.
They're broadly divided into conformance tests, benchmarking, and helpers.

Most of the tools in the *afptest* family follow the same general usage
pattern and parameters. You set the AFP protocol version (**-1** through
**-7**), then the address and credentials of the host to test (which can
be localhost). Some tests require a second user and second volume to be
defined. Yet another set of tests must be run from localhost, and the
local path to the volume under test to be provided. Single tests or test
sections can be executed with the **-f** option. Available tests can be
listed with the **-l** option.

Please refer to the helptext of each tool for the precise use of its
options.

## Return codes

Each test within a testsuite returns one of the following return codes:

- 0 PASSED
- 1 FAILED
- 2 NOT TESTED - a test setup step or precondition check failed
- 3 SKIPPED - unmet requirements for testing

Note that a NOT TESTED result is treated as a failure of the entire test run,
but SKIPPED is not.

The spectest and logintest shall return the same results whether they are run
against a Mac AFP server or Netatalk.

## Conformance tests

**afp_spectest** makes up the core of the AFP specification test suite,
with several hundred test cases. It is organized into testsets, divided by
the AFP commands tested, or by preconditions for testing. For instance, the
tier 2 (T2) tests need to be run on the host with the **-c** option
indicating the path to the shared volume. There are also read-only and
sleep tests that need to be run separately.

**afp_logintest** is a testsuite for DSI sessions and authentication.

## Benchmarking

**afp_lantest** is a file transfer benchmarking tool for AFP
servers, inspired by *HELIOS LanTest*, which runs a batch
of varied file transfer patterns.

**afp_speedtest** is a benchmark testsuite for read, write and copy
operations. It can be run using either AFP commands or POSIX syscalls,
handy for comparing netatalk speeds against other file transfer protocols.

## Helpers

**afparg** is an interactive AFP client that takes an AFP command with
optional arguments. This can be used for troubleshooting or system
administration. Run *afparg -l* to list available commands.

**fce_listen** is a simple listener for Netatalk's Filesystem Change Event
(FCE) protocol. It will print out any UDP datagrams received from the AFP
server.

## Testing a Mac AFP server

This suite of tools were designed primarily to test Netatalk AFP servers,
however they can also be used to test a native Mac OS AFP server hosted
by an older Mac OS X or Classic Mac OS system.

Launch the test runner with the **-m** option when testing a Mac AFP server.
When running in Mac mode, the test runner will report tests with known current
or historical differences between Mac and Netatalk.

If Mac and Netatalk differ, or if Mac results differ between versions:

    header.dsi_code       -5000     AFPERR_ACCESS
    MAC RESULT: -5019 AFPERR_PARAM    -5010 AFPERR_BUSY
    Netatalk returns AFPERR_ACCESS when a Mac return AFPERR_PARAM or AFPERR_BUSY

When Mac and Netatalk historically returned different results
but now behave the same way:

    Warning MAC and Netatalk now same RESULT!

# Examples

## Configure environment

Below is a sample configuration for running the AFP spec tests.

- 2 users: user1, user2 with the same password
- 1 group: afpusers
- user1, user2 assigned to afpusers group
- clear text UAM + guest UAM

Arrange two *empty* directories. Some tests will fail if there are
residual files in the test directories.

    drwxrwsr-x    5 user1   afpusers       176 avr 27 23:56 /tmp/afptest1
    drwxrwsr-x    5 user1   afpusers       176 avr 27 23:56 /tmp/afptest2

Set afp.conf as follows:

    [Global]
    uam list = uams_clrtxt.so uams_guest.so

    [testvol1]
    ea = sys
    path = /tmp/afptest1
    valid users = @afpusers
    volume name = testvol1

    [testvol2]
    ea = sys
    path = /tmp/afptest2
    valid users = @afpusers
    volume name = testvol2

## Running tests

Run the afp_spectest for the "FPSetForkParms_test" testset with AFP 3.4.

    % afp_spectest -h 127.0.0.1 -p 548 -u user1 -d user2 -w passwd -s testvol1 -S testvol2 -c /srv/afptest1 -7 -f FPSetForkParms_test
    ===================
    FPSetForkParms_test
    -------------------
    FPSetForkParms:test62: SetForkParams errors - PASSED
    FPSetForkParms:test141: Setforkmode error - PASSED
    FPSetForkParms:test217: Setfork size 64 bits - PASSED
    FPSetForkParms:test306: set fork size, new size > old size - PASSED

Run the afp_lantest benchmark using AFP 3.0.

    % afp_lantest -h 192.168.0.2 -s testvol1 -u user1 -w passwd -3
    Run 0 => Opening, stating and reading 512 bytes from 1000 files   1799 ms
    Run 0 => Writing one large file                                     30 ms for 100 MB (avg. 3495 MB/s)
    Run 0 => Reading one large file                                      8 ms for 100 MB (avg. 13107 MB/s)
    Run 0 => Locking/Unlocking 10000 times each                       1959 ms
    Run 0 => Creating dir with 2000 files                             1339 ms
    Run 0 => Enumerate dir with 2000 files                             217 ms
    Run 0 => Create directory tree with 10^3 dirs                      496 ms

    Netatalk Lantest Results (averages)
    ===================================

    Opening, stating and reading 512 bytes from 1000 files   1799 ms
    Writing one large file                                     30 ms for 100 MB (avg. 3495 MB/s)
    Reading one large file                                      8 ms for 100 MB (avg. 13107 MB/s)
    Locking/Unlocking 10000 times each                       1959 ms
    Creating dir with 2000 files                             1339 ms
    Enumerate dir with 2000 files                             217 ms
    Create directory tree with 10^3 dirs                      496 ms

# See also

afpd(8)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
