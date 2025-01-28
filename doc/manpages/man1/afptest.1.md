# Name

afp_lantest, afp_logintest, afp_spectest, afp_speedtest, afparg — AFP protocol tests

# Synopsis

`afp_lantest [-34567GgVv] [-h host] [-p port] [-s volume] [-u user] [-w password] [-n iterations] [-t tests] [-F bigfile]`

`afp_logintest [-1234567CmVv] [-h host] [-p port] [-s volume] [-u user] [-w password]`

`afp_spectest [-1234567aCiLlmVvXx] [-h host] [-H host2] [-p port] [-s volume] [-c path to volume] [-S volume2] [-u user] [-d user2] [-w password] [-f test]`

`afp_speedtest [-1234567aeiLnVvy] [-h host] [-p port] [-s volume] [-S volume2] [-u user] [-w password] [-n iterations] [-d size] [-q quantum] [-F file] [-f test]`

`afparg [-1234567lVv] [-h host] [-p port] [-s volume] [-u user] [-w password] [-f command]`

# Description

All of the tools in the *afptest* family follow the same general usage
pattern and parameters. You set the AFP protocol revision (**-1** through
**-7**), then the address and credentials of the host to test (which can
be localhost). Some tests require a second user and second volume to be
define. Yet another set of tests must be run from localhost, and the
local path to the volume under test to be provided. Single tests or test
sections can be executed with the **-f** option. Available tests can be
listed with the **-l** option.

**afp_spectest** makes up the core of the AFP specification test suite,
with just over 300 test cases. It is organized into testsets, divided by
AFP commands tested, or by preconditions for testing. For instance, the
tier 2 (T2) tests need to be run on the host with the **-c** option
indicating the path to the shared volume. There are also read-only and
sleep tests that need to be run separately.

**afp_logintest** is an AFP login authentication test suite that has its
own runners.

**afp_lantest** and **afp_speedtest**` are file transfer benchmarks for AFP
servers. The former is inspired by *HELIOS LanTest*, which runs a batch
of varied file transfer patterns. The latter is a simpler tool with a
handful of available test cases.

**afparg**` is an AFP CLI client that takes a specific command with
optional arguments, and sends a single action to the AFP server. This
can be used for one-off troubleshooting or system administration.

Please refer to the helptext of each tool for the precise use of each
option.

# Examples

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
