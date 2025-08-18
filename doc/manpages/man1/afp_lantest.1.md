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
: Run cache-focused tests only (tests 9-12)

**-n** *iterations*
: Number of test iterations to run (default: 2). For iterations > 5 outliers will be removed

**-p** *port*
: Server port number (default: 548)

**-s** *volume*
: Volume name to mount for testing

**-u** *user*
: Username for authentication (default: current uid)

**-V**
: Very verbose output

**-v**
: Verbose output

**-w** *password*
: Password for authentication

# Configuration

The test runner AFP client only supports the ClearTxt UAM currently.
Configure the UAM in netatalk's afp.conf:

    [Global]
    uam list = uams_clrtxt.so

# Available Tests

The following tests are available:

**(1) Opening, stating and reading 512 bytes from 1000 files**
: Tests basic file access patterns with many small operations

**(2) Writing one large file**
: Measures sustained write performance

**(3) Reading one large file**
: Measures sustained read performance

**(4) Locking/Unlocking 10000 times each**
: Tests file locking performance

**(5) Creating dir with 2000 files**
: Tests directory creation and file creation performance

**(6) Enumerate dir with 2000 files**
: Tests directory enumeration with many files

**(7) Deleting dir with 2000 files**
: Tests directory and file deletion performance

**(8) Create directory tree with 1000 dirs**
: Tests nested directory creation

**(9) Directory cache hits [CACHE]**
: Tests directory and file lookup performance (1000 dirs + 10000 files)

**(10) Mixed cache operations [CACHE]**
: Tests mixed create/stat/enum/delete operations

**(11) Deep path traversal [CACHE]**
: Tests performance with deep directory structures

**(12) Cache validation [CACHE]**
: Tests directory cache validation mechanisms

## Cache-Focused Tests

Tests 9-12 are specifically designed to highlight directory cache performance improvements in
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
Set gid to the group ID of the user running afp_lantest.
For example, to run as root;
`mkdir -p /proc_io && mount -t proc -o hidepid=0,gid=0 proc /proc_io`

    afp_lantest -n 2 -7 -h 127.0.0.1 -p 548 -u test -w test -s 'File Sharing'
    Connecting to host 127.0.0.1:548
    IO monitoring: /proc_io is available
    Looking for cnid_dbd processes with -u test in command line
    Found cnid_dbd process: PID 40
    Looking for afpd processes owned by user 'test' (UID: 1000)
    Found privilege-dropped afpd process: PID 36
    IO monitoring enabled (afpd: 36, cnid_dbd: 40)

    Run 1 => Open, stat and read 512 bytes from 1000 files [8,000 AFP ops]        1923 ms
            IO Operations; afpd: 6000 READs, 7002 WRITEs | cnid_dbd: 0 READs, 2 WRITEs
    Run 1 => Writing one large file [103 AFP ops]                                  136 ms for 100 MB (avg. 771 MB/s)
            IO Operations; afpd: 0 READs, 299 WRITEs | cnid_dbd: 0 READs, 0 WRITEs
    Run 1 => Reading one large file [102 AFP ops]                                   39 ms for 100 MB (avg. 2688 MB/s)
            IO Operations; afpd: 100 READs, 100 WRITEs | cnid_dbd: 0 READs, 0 WRITEs
    Run 1 => Locking/Unlocking 10000 times each [20,000 AFP ops]                   799 ms
            IO Operations; afpd: 0 READs, 20000 WRITEs | cnid_dbd: 0 READs, 0 WRITEs
    Run 1 => Creating dir with 2000 files [4,000 AFP ops]                         4061 ms
            IO Operations; afpd: 2000 READs, 10005 WRITEs | cnid_dbd: 4 READs, 6150 WRITEs
    Run 1 => Enumerate dir with 2000 files [~51 AFP ops]                           637 ms
            IO Operations; afpd: 1960 READs, 49 WRITEs | cnid_dbd: 0 READs, 0 WRITEs
    Run 1 => Deleting dir with 2000 files [2,000 AFP ops]                         3176 ms
            IO Operations; afpd: 4000 READs, 4004 WRITEs | cnid_dbd: 2 READs, 6104 WRITEs
    Run 1 => Create directory tree with 1000 dirs [1,110 AFP ops]                 1885 ms
            IO Operations; afpd: 0 READs, 4445 WRITEs | cnid_dbd: 4 READs, 2351 WRITEs
    Run 1 => Directory cache hits (100 dirs + 1000 files) [11,000 AFP ops]        3625 ms
            IO Operations; afpd: 10000 READs, 11100 WRITEs | cnid_dbd: 0 READs, 100 WRITEs
    Run 1 => Mixed cache operations (create/stat/enum/delete) [820 AFP ops]       1134 ms
            IO Operations; afpd: 820 READs, 1621 WRITEs | cnid_dbd: 0 READs, 1201 WRITEs
    Run 1 => Deep path traversal (nested directory navigation) [3,500 AFP ops]     965 ms
            IO Operations; afpd: 2500 READs, 3550 WRITEs | cnid_dbd: 0 READs, 50 WRITEs
    Run 1 => Cache validation efficiency (metadata changes) [30,000 AFP ops]      8529 ms
            IO Operations; afpd: 30000 READs, 30100 WRITEs | cnid_dbd: 0 READs, 100 WRITEs
    Run 2 => Open, stat and read 512 bytes from 1000 files [8,000 AFP ops]        2453 ms
            IO Operations; afpd: 6000 READs, 7002 WRITEs | cnid_dbd: 0 READs, 2 WRITEs
    Run 2 => Writing one large file [103 AFP ops]                                   87 ms for 100 MB (avg. 1205 MB/s)
            IO Operations; afpd: 0 READs, 299 WRITEs | cnid_dbd: 0 READs, 0 WRITEs
    Run 2 => Reading one large file [102 AFP ops]                                   36 ms for 100 MB (avg. 2912 MB/s)
            IO Operations; afpd: 100 READs, 100 WRITEs | cnid_dbd: 0 READs, 0 WRITEs
    Run 2 => Locking/Unlocking 10000 times each [20,000 AFP ops]                   769 ms
            IO Operations; afpd: 0 READs, 20000 WRITEs | cnid_dbd: 0 READs, 0 WRITEs
    Run 2 => Creating dir with 2000 files [4,000 AFP ops]                         3442 ms
            IO Operations; afpd: 2000 READs, 10005 WRITEs | cnid_dbd: 7 READs, 6140 WRITEs
    Run 2 => Enumerate dir with 2000 files [~51 AFP ops]                           805 ms
            IO Operations; afpd: 1960 READs, 49 WRITEs | cnid_dbd: 0 READs, 0 WRITEs
    Run 2 => Deleting dir with 2000 files [2,000 AFP ops]                         2475 ms
            IO Operations; afpd: 4000 READs, 4003 WRITEs | cnid_dbd: 4 READs, 6180 WRITEs
    Run 2 => Create directory tree with 1000 dirs [1,110 AFP ops]                 1701 ms
            IO Operations; afpd: 0 READs, 4442 WRITEs | cnid_dbd: 2 READs, 2267 WRITEs
    Run 2 => Directory cache hits (100 dirs + 1000 files) [11,000 AFP ops]        2962 ms
            IO Operations; afpd: 10000 READs, 11100 WRITEs | cnid_dbd: 0 READs, 100 WRITEs
    Run 2 => Mixed cache operations (create/stat/enum/delete) [820 AFP ops]        598 ms
            IO Operations; afpd: 820 READs, 1621 WRITEs | cnid_dbd: 2 READs, 1242 WRITEs
    Run 2 => Deep path traversal (nested directory navigation) [3,500 AFP ops]     796 ms
            IO Operations; afpd: 2500 READs, 3550 WRITEs | cnid_dbd: 0 READs, 50 WRITEs
    Run 2 => Cache validation efficiency (metadata changes) [30,000 AFP ops]      8431 ms
            IO Operations; afpd: 30000 READs, 30100 WRITEs | cnid_dbd: 0 READs, 100 WRITEs

    Netatalk Lantest Results (Averages and standard deviations (±) for all tests, across 2 iterations (default))
    ============================================================================================================

    Test                                                                Time_ms  Time± AFPD_R AFPD_R± AFPD_W AFPD_W± CNID_R CNID_R± CNID_W CNID_W±   MB/s
    ------------------------------------------------------------------ -------- ------ ------ ------- ------ ------- ------ ------- ------ ------- ------
    Open, stat and read 512 bytes from 1000 files [8,000 AFP ops]          2188  374.8   6000     0.0   7002     0.0      0     0.0      2     0.0      0
    Writing one large file [103 AFP ops]                                    111   34.7      0     0.0    299     0.0      0     0.0      0     0.0    900
    Reading one large file [102 AFP ops]                                     37    2.2    100     0.0    100     0.0      0     0.0      0     0.0   2702
    Locking/Unlocking 10000 times each [20,000 AFP ops]                     784   21.2      0     0.0  20000     0.0      0     0.0      0     0.0      0
    Creating dir with 2000 files [4,000 AFP ops]                           3751  437.7   2000     0.0  10005     0.0      5     2.2   6145     7.1      0
    Enumerate dir with 2000 files [~51 AFP ops]                             721  118.8   1960     0.0     49     0.0      0     0.0      0     0.0      0
    Deleting dir with 2000 files [2,000 AFP ops]                           2825  495.7   4000     0.0   4003     1.0      3     1.4   6142    53.7      0
    Create directory tree with 1000 dirs [1,110 AFP ops]                   1793  130.1      0     0.0   4443     2.2      3     1.4   2309    59.4      0
    Directory cache hits (100 dirs + 1000 files) [11,000 AFP ops]          3293  468.8  10000     0.0  11100     0.0      0     0.0    100     0.0      0
    Mixed cache operations (create/stat/enum/delete) [820 AFP ops]          866  379.0    820     0.0   1621     0.0      2     0.0   1221    29.0      0
    Deep path traversal (nested directory navigation) [3,500 AFP ops]       880  119.5   2500     0.0   3550     0.0      0     0.0     50     0.0      0
    Cache validation efficiency (metadata changes) [30,000 AFP ops]        8480   69.3  30000     0.0  30100     0.0      0     0.0    100     0.0      0
    ------------------------------------------------------------------ -------- ------ ------ ------- ------ ------- ------ ------- ------ ------- ------
    Sum of all AFP OPs = 80686                                            25729         57380          92272             13          16069               

    Aggregates Summary:
    -------------------
    Average Time per AFP OP: 0.319 ms
    Average AFPD Reads per AFP OP: 0.711
    Average AFPD Writes per AFP OP: 1.144

## Result Columns

    Time(ms) = Test runtime in milliseconds
    Time±    = Test runtime standard deviation
    AFPD_R   = afpd process IO Read operations
    AFPD_R±  = afpd process IO Read operation standard deviation
    AFPD_W   = afpd process IO Write operations
    AFPD_W±  = afpd process IO Write operation standard deviation

    CNID_*   = IO measurements for the cnid_dbd process (optional)

## Aggregates Summary

The aggregate values are purely Intrinsic Metrics, as AFP operations are a mixture of reads, writes,
and connection related operations.

Therefore the measurements are meaningful only within their own context.
They provide a way to assess changes relative to itself,
but lack any external coherence or reference point for comparison with other systems.

Generally, values less than 1 indicate efficient operation (for example due to batching and caching etc),
and values greater than 1 indicate sub-optimal operation (for example amplification of a single operation,
causing additional downstream operations).

Note; As total AFP Ops are a mix of reads, writes, and other ops the effective 1:1 (AFP:IO) thresholds are not equal to 1.
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
