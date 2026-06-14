# Name

afp_lantest — AFP LAN performance and directory cache testing tool

# Synopsis

**afp_lantest** [-34567bcGgKVv] [-h *host*] [-p *port*] [-s *volume*] [-u *user*] [-w *password*]
[-n *iterations*] [-f *tests*] [-F *bigfile*]

# Description

**afp_lantest** is a comprehensive AFP (Apple Filing Protocol) performance testing tool designed to
benchmark various aspects of AFP servers, including directory cache performance in netatalk.

The tool runs a series of tests that measure file operations, directory traversal, and caching
efficiency. It includes both traditional file system benchmarks and specialized cache-focused tests
that highlight the benefits of optimized directory cache validation and probabilistic validation
features.

# Options

**-3**
: Use AFP 3.0 protocol version

**-4**
: Use AFP 3.1 protocol version

**-5**
: Use AFP 3.2 protocol version

**-6**
: Use AFP 3.3 protocol version

**-7**
: Use AFP 3.4 protocol version (default)

**-b**
: Debug mode

**-c**
: Output results in CSV format (default: tabular)

**-F** *bigfile*
: Use existing file in volume root for read test (file size must match -g/-G options)

**-f** *tests*
: Specific tests to run, specified as digits (e.g., "134" runs tests 1, 3, and 4)

**-G**
: Optimize for 10-gigabit networks (increases file test size to 10 GB)

**-g**
: Optimize for gigabit networks (increases file test size to 1 GB)

**-h** *host*
: Server hostname or IP address (default: localhost)

**-K**
: Run cache-focused tests only (tests 11-14)

**-n** *iterations*
: Number of test iterations to run (default: 2). For iterations > 5 outliers will be removed

**-p** *port*
: Server port number (default: 548)

**-s** *volume*
: Volume name to mount for testing

**-u** *user*
: Username for authentication with AFP server (default: current uid)

**-V**
: Very verbose output

**-v**
: Verbose output

**-w** *password*
: Password for authentication with AFP server

# Configuration

The test runner AFP client only supports the ClearTxt UAM currently.
Configure the UAM in netatalk's afp.conf:

    [Global]
    uam list = uams_clrtxt.so

# Available Tests

The following tests are available:

Most tests operate on a shared set of 2000 files, normalized so their timings
are comparable. Tests 3-8 form a pipeline over the same 2000 files (create,
write, lock, read, enumerate, delete); selecting any of them implies test 3.

**(1) Writing one large file**
: Measures sustained write performance

**(2) Reading one large file**
: Measures sustained read performance

**(3) Creating 2000 files**
: Tests file creation performance

**(4) Writing 1024 bytes to 2000 files**
: Tests write performance over many small files (implies test 3)

**(5) Lock then unlock 2000 open forks**
: Tests refnum lookup and lock setup against a full open-fork table (implies test 3)

**(6) Stat, open, read 512 bytes, close 2000 files**
: Tests basic file access patterns with many small operations (implies tests 3, 4)

**(7) Enumerate dir with 2000 files**
: Tests directory enumeration with many files (implies test 3)

**(8) Deleting 2000 files**
: Tests file deletion performance (implies test 3)

**(9) Byte-range lock/unlock 2000 ranges in one fork**
: Tests per-fork byte-range lock tracking with many locks held at once

**(10) Create directory tree with 1000 dirs**
: Tests nested directory creation (10 x 10 x 10)

**(11) Directory cache hits [CACHE]**
: Tests directory and file lookup performance (20 dirs x 100 files)

**(12) Mixed cache operations [CACHE]**
: Tests mixed create/stat/enum/delete operations over 1000 files

**(13) Deep path traversal [CACHE]**
: Tests 100 walks of a 20-level deep directory tree

**(14) Cache validation [CACHE]**
: Tests cache revalidation over 2000 files (3 metadata lookups each)

## Cache-Focused Tests

Tests 11-14 are specifically designed to highlight directory cache performance improvements in
netatalk. These tests benefit significantly from optimized cache validation and probabilistic
validation features. Use the **-K** option to run only these cache-focused tests.

# Examples

Run all tests with default settings:

    afp_lantest -h server.example.com -u testuser -w password -s TestVolume

Run only cache-focused tests:

    afp_lantest -K -h server.example.com -u testuser -w password -s TestVolume

Run specific tests (file operations):

    afp_lantest -f 123 -h server.example.com -u testuser -w password -s TestVolume

Run tests optimized for gigabit network:

    afp_lantest -g -h server.example.com -u testuser -w password -s TestVolume

Run multiple iterations for statistical analysis:

    afp_lantest -n 5 -h server.example.com -u testuser -w password -s TestVolume

# Example with IO Monitoring

IO Monitoring (Linux only) requires proc filesystem mounted at /proc_io with hidepid=0.
Set *gid* to the group ID of the user running afp_lantest.

For example, to run as root;
`mkdir -p /proc_io && mount -t proc -o hidepid=0,gid=0 proc /proc_io`

    afp_lantest -n 2 -7 -h 127.0.0.1 -p 548 -u test -w test -s 'File Sharing'
    Connecting to host 127.0.0.1:548
    IO monitoring: /proc_io is available
    Found cnid_dbd process for user 'test': PID 40
    Found privilege-dropped afpd process for user 'test': PID 36
    IO monitoring enabled (afpd: 36, cnid_dbd: 40)

    Run 1 => Writing one large file [103 AFP ops]                                  208 ms for 100 MB (avg. 504 MB/s)
            IO Operations; afpd: 101 READs, 301 WRITEs | cnid_dbd: 0 READs, 0 WRITEs
    Run 1 => Reading one large file [102 AFP ops]                                   39 ms for 100 MB (avg. 2688 MB/s)
            IO Operations; afpd: 201 READs, 100 WRITEs | cnid_dbd: 0 READs, 0 WRITEs
    Run 1 => Creating 2000 files [4,000 AFP ops]                                  2847 ms
            IO Operations; afpd: 4000 READs, 10000 WRITEs | cnid_dbd: 4 READs, 4151 WRITEs
    Run 1 => Writing 1024 bytes to 2000 files [6,000 AFP ops]                     2122 ms
            IO Operations; afpd: 8000 READs, 8000 WRITEs | cnid_dbd: 0 READs, 0 WRITEs
    Run 1 => Lock then unlock 2000 open forks [4,000 AFP ops]                      133 ms
            IO Operations; afpd: 4000 READs, 4000 WRITEs | cnid_dbd: 0 READs, 0 WRITEs
    Run 1 => Stat, open, read 512 bytes, close 2000 files [16,000 AFP ops]        3864 ms
            IO Operations; afpd: 26000 READs, 14000 WRITEs | cnid_dbd: 0 READs, 0 WRITEs
    Run 1 => Enumerate dir with 2000 files [~51 AFP ops]                             9 ms
            IO Operations; afpd: 49 READs, 49 WRITEs | cnid_dbd: 0 READs, 0 WRITEs
    Run 1 => Deleting 2000 files [2,000 AFP ops]                                  2705 ms
            IO Operations; afpd: 6000 READs, 8000 WRITEs | cnid_dbd: 2 READs, 6100 WRITEs
    Run 1 => Byte-range lock/unlock 2000 ranges in one fork [4,000 AFP ops]        160 ms
            IO Operations; afpd: 4000 READs, 4000 WRITEs | cnid_dbd: 0 READs, 0 WRITEs
    Run 1 => Create directory tree with 1000 dirs [1,110 AFP ops]                 1755 ms
            IO Operations; afpd: 1110 READs, 5550 WRITEs | cnid_dbd: 4 READs, 2284 WRITEs
    Run 1 => Directory cache hits (20 dirs x 100 files) [2,020 AFP ops]             80 ms
            IO Operations; afpd: 2020 READs, 2020 WRITEs | cnid_dbd: 0 READs, 0 WRITEs
    Run 1 => Mixed cache operations (create/stat/enum/delete) on 1000 files [4,100 AFP ops]  3633 ms
            IO Operations; afpd: 6101 READs, 10101 WRITEs | cnid_dbd: 8 READs, 5025 WRITEs
    Run 1 => Deep path traversal (20 levels x 100 walks) [2,000 AFP ops]            81 ms
            IO Operations; afpd: 2000 READs, 2000 WRITEs | cnid_dbd: 0 READs, 0 WRITEs
    Run 1 => Cache validation (2000 files x 3 lookups) [6,000 AFP ops]             244 ms
            IO Operations; afpd: 6000 READs, 6000 WRITEs | cnid_dbd: 0 READs, 0 WRITEs
    Successfully deleted test directory 'LanTest-1'

    Netatalk Lantest Results (Averages and standard deviations (±) for all tests, across 10 iterations)
    ============================================================================================================

    Test                                                                Time_ms  Time± AFPD_R AFPD_R± AFPD_W AFPD_W± CNID_R CNID_R± CNID_W CNID_W±   MB/s
    ------------------------------------------------------------------ -------- ------ ------ ------- ------ ------- ------ ------- ------ ------- ------
    Writing one large file [103 AFP ops]                                    172   55.8    101     0.0    300     0.0      0     0.0      0     0.0    581
    Reading one large file [102 AFP ops]                                     39    3.9    201     0.0    100     0.0      0     0.0      0     0.0   2564
    Creating 2000 files [4,000 AFP ops]                                    3144  121.4   4000     0.0  10000     0.0      4     0.0   4133     7.3      0
    Writing 1024 bytes to 2000 files [6,000 AFP ops]                       2030  230.0   8000     0.0   8000     0.0      0     0.0      0     0.0      0
    Lock then unlock 2000 open forks [4,000 AFP ops]                        165    8.4   4000     0.0   4000     0.0      0     0.0      0     0.0      0
    Stat, open, read 512 bytes, close 2000 files [16,000 AFP ops]          3420  342.4  26000     0.0  14000     0.0      0     0.0      0     0.0      0
    Enumerate dir with 2000 files [~51 AFP ops]                              11    1.6     49     0.0     49     0.0      0     0.0      0     0.0      0
    Deleting 2000 files [2,000 AFP ops]                                    2691  239.8   6000     0.0   8000     0.0      4     1.1   6177     3.5      0
    Byte-range lock/unlock 2000 ranges in one fork [4,000 AFP ops]          172   12.2   4000     0.0   4000     0.0      0     0.0      0     0.0      0
    Create directory tree with 1000 dirs [1,110 AFP ops]                   1877  109.5   1110     0.0   5550     0.0      4     1.4   2280     8.0      0
    Directory cache hits (20 dirs x 100 files) [2,020 AFP ops]               93   11.1   2020     0.0   2020     0.0      0     0.0      0     0.0      0
    Mixed cache operations (create/stat/enum/delete) on 1000 files [4,100 AFP ops]     3273  468.0   6100     0.5  10100     0.5      8     1.6   5033    14.2      0
    Deep path traversal (20 levels x 100 walks) [2,000 AFP ops]              84    3.3   2000     0.0   2000     0.0      0     0.0      0     0.0      0
    Cache validation (2000 files x 3 lookups) [6,000 AFP ops]               242    9.4   6000     0.0   6000     0.0      0     0.0      0     0.0      0
    ------------------------------------------------------------------ -------- ------ ------ ------- ------ ------- ------ ------- ------ ------- ------
    Sum of all AFP OPs = 51486                                            17413         69581          74119             20          17623

    Aggregates Summary:
    ------------------------------------------------------------------
    Average Time per AFP OP: 0.538 ms (from per-test medians)
    Average AFPD Reads per AFP OP: 0.711
    Average AFPD Writes per AFP OP: 1.144
    See afp_lantest manpage for more information: https://netatalk.io/manual/en/afp_lantest.1

    Dircache Statistics (/var/log/afpd.log):
    ------------------------------------------------------------------
    Sep 24 13:30:15.702673 afpd[36] {dircache.c:632} (info:AFPDaemon): dircache statistics: (user: test) entries: 0, lookups: 244476, hits: 227977 (93.3%), misses: 9228, added: 9552, removed: 9552, expunged: 7271, evicted: 0

## IO Monitoring Result Columns

    Time(ms) = Test runtime in milliseconds
    Time±    = Test runtime standard deviation
    AFPD_R   = afpd process IO Read operations
    AFPD_R±  = afpd process IO Read operation standard deviation
    AFPD_W   = afpd process IO Write operations
    AFPD_W±  = afpd process IO Write operation standard deviation

    CNID_*   = IO measurements for the cnid_dbd process (optional)

Note; When measuring afpd read/write IO with afp_lantest, ensure afp.conf `log level` is set to `default:error`

- More verbose logging superficially increases *_W IO values. For dirstats at least `default:info` is reqired.

## IO Monitoring Aggregates Summary

The aggregate values are purely Intrinsic Metrics, as AFP operations are a mixture of reads, writes,
and connection related operations.

Therefore the measurements are meaningful only within their own context.
They provide a way to assess changes relative to itself,
but lack any external coherence or reference point for comparison with other systems.

Generally, values less than 1 indicate efficient operation (for example due to batching and caching etc),
and values greater than 1 indicate sub-optimal operation (for example amplification of a single operation,
causing additional downstream operations).

Note; As total AFP Ops are a mix of reads, writes,
and other ops the effective 1:1 (AFP:IO) thresholds are not equal to 1.
Accurate thresholds can be calculated on a per test basis using the information shown in the Testing Wiki,
and comparing relevant AFP read/write ops with the relevant AFPD_R/AFPD_W values.

Ideally code changes should observe aggregate values as reducing.

For more information see [Testing Wiki](https://netatalk.io/docs/Testing)

# Return Codes

**0**
: All tests passed successfully

**1**
: One or more tests failed

# Notes

- Tests that involve large file operations (tests 2 and 3) will create temporary files in the
  test volume
- The **-g** and **-G** options significantly increase test file sizes and test duration
- Cache-focused tests (8-11) provide the most insight into netatalk's directory cache performance
- Multiple iterations (**-n**) are recommended for consistent performance measurements
- The tool requires write access to the specified AFP volume

# See Also

**afp_speedtest**(1), **afpd**(8), **netatalk**(8)
