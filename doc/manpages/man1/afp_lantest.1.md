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
: Run cache-focused tests only (tests 14-17)

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
are comparable. Tests 3-11 form a pipeline over the same 2000 files (create,
write, read, copy, server-copy, stat, enumerate, lock, delete); selecting any
of them implies test 3.

Each test's AFP operation count below covers only the timed region; setup and
cleanup operations (marked untimed) are excluded from the timing and the count.

**(1) Writing one large file**
: Measures sustained write performance. Timed: ~100 quantum-sized FPWriteExt
calls plus the final FPCloseFork [103 AFP ops]. Untimed setup: FPCreateFile,
FPGetFileDirParms x2, FPOpenFork, FPGetForkParms.

**(2) Reading one large file**
: Measures sustained read performance. Timed: ~100 quantum-sized FPReadExt
calls plus the final FPCloseFork [102 AFP ops]. Untimed setup:
FPGetFileDirParms, FPOpenFork, FPGetForkParms.

**(3) Creating 2000 files**
: Tests file creation performance. Timed, per file: FPCreateFile +
FPGetFileDirParms [2 x 2000 = 4,000 AFP ops].

**(4) Open, write 1024 bytes, close 2000 files**
: Tests the full small-write lifecycle per file (implies test 3). Timed, per
file: FPOpenFork + FPWrite + FPCloseFork [3 x 2000 = 6,000 AFP ops].

**(5) Open, read 512 bytes, close 2000 files**
: Tests the full small-read lifecycle a real client pays per file, including
the lock management carried by each open and close (implies tests 3, 4).
Mirrors test 4's timed open+write+close so the read and write results are
directly comparable. Timed, per file: FPOpenFork + FPRead + FPCloseFork
[3 x 2000 = 6,000 AFP ops].

**(6) Copying 2000 files client-side**
: Tests copying where the client reads each file's data and writes it back to
a new file, as file managers without server-side copy do (implies tests 3, 4).
Timed, per file: FPOpenFork + FPRead + FPCloseFork on the source, then
FPCreateFile + FPOpenFork + FPWrite + FPCloseFork on the copy
[7 x 2000 = 14,000 AFP ops]. Untimed cleanup: FPDelete per copy.

**(7) Copying 2000 files server-side**
: Tests FPCopyFile, where the data never crosses the wire (implies tests 3, 4).
Timed, per file: FPCopyFile [1 x 2000 = 2,000 AFP ops]. Untimed cleanup:
FPDelete per copy. Contrast with test 6 to see the round-trip savings.

**(8) Stat 2000 files**
: Tests file lookup and metadata query performance (implies test 3). Timed,
per file: three FPGetFileDirParms requests with varying bitmaps
[3 x 2000 = 6,000 AFP ops].

**(9) Enumerate dir with 2000 files**
: Tests directory enumeration with many files (implies test 3). Timed:
FPEnumerateExt2 in 40-entry chunks over the 2000-file directory
[~51 AFP ops].

**(10) Lock then unlock 2000 open forks**
: Tests refnum lookup and lock setup against a full open-fork table (implies
test 3). Timed: FPByteRangeLock lock + unlock on each fork
[2 x 2000 = 4,000 AFP ops]. Untimed: FPOpenFork x2000 setup,
FPCloseFork x2000 cleanup.

**(11) Deleting 2000 files**
: Tests file deletion performance (implies test 3). Timed, per file: FPDelete
[1 x 2000 = 2,000 AFP ops].

**(12) Byte-range lock/unlock 2000 ranges in one fork**
: Tests per-fork byte-range lock tracking with many locks held at once. Timed:
FPByteRangeLock lock x2000 then unlock x2000 on distinct ranges in a single
fork [4,000 AFP ops]. Untimed: file create/write/open setup and cleanup.

**(13) Create directory tree with 1000 dirs**
: Tests nested directory creation (10 x 10 x 10). Timed: FPCreateDir per
directory [1,110 AFP ops]. Untimed cleanup: FPDelete per directory.

**(14) Directory cache hits [CACHE]**
: Tests directory and file lookup performance (20 dirs x 100 files). Timed:
FPGetFileDirParms per directory and per file [2,020 AFP ops]. Untimed:
FPCreateDir/FPCreateFile setup, FPDelete cleanup.

**(15) Mixed cache operations [CACHE]**
: Tests the cache lifecycle over 1000 files (implies nothing; self-contained).
Timed, per file: FPCreateFile + FPGetFileDirParms + FPGetFileDirParms +
FPDelete, plus FPEnumerate every 10th file [4 x 1000 + 100 = 4,100 AFP ops].

**(16) Deep path traversal [CACHE]**
: Tests 100 walks of a 20-level deep directory tree. Timed: FPGetFileDirParms
per level per walk [20 x 100 = 2,000 AFP ops]. Untimed: FPCreateDir setup,
FPDelete cleanup.

**(17) Cache validation [CACHE]**
: Tests cache revalidation over 2000 files. Timed, per file: three
FPGetFileDirParms metadata lookups [3 x 2000 = 6,000 AFP ops]. Untimed:
FPCreateFile setup, FPDelete cleanup.

## Cache-Focused Tests

Tests 14-17 are specifically designed to highlight directory cache performance improvements in
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
    Found cnid_dbd process for user 'test': PID 46
    Found privilege-dropped afpd process for user 'test': PID 41
    IO monitoring enabled (afpd: 41, cnid_dbd: 46)

    Run 1 => Writing one large file [103 AFP ops]                                   42 ms for 100 MB (avg. 2496 MB/s)
             IO Operations; afpd: 101 READs, 301 WRITEs | cnid_dbd: 0 READs, 0 WRITEs
    Run 1 => Reading one large file [102 AFP ops]                                   19 ms for 100 MB (avg. 5518 MB/s)
             IO Operations; afpd: 277 READs, 176 WRITEs | cnid_dbd: 0 READs, 0 WRITEs
    Run 1 => Creating 2000 files [4,000 AFP ops]                                   444 ms
             IO Operations; afpd: 4000 READs, 6000 WRITEs | cnid_dbd: 4 READs, 4151 WRITEs
    Run 1 => Open, write 1024 bytes, close 2000 files [6,000 AFP ops]              267 ms
             IO Operations; afpd: 6000 READs, 8000 WRITEs | cnid_dbd: 0 READs, 0 WRITEs
    Run 1 => Open, read 512 bytes, close 2000 files [6,000 AFP ops]                258 ms
             IO Operations; afpd: 8000 READs, 4000 WRITEs | cnid_dbd: 0 READs, 0 WRITEs
    Run 1 => Copying 2000 files client-side [14,000 AFP ops]                       726 ms
             IO Operations; afpd: 16000 READs, 16000 WRITEs | cnid_dbd: 2 READs, 4101 WRITEs
    Run 1 => Copying 2000 files server-side [2,000 AFP ops]                        503 ms
             IO Operations; afpd: 6000 READs, 8000 WRITEs | cnid_dbd: 4 READs, 4143 WRITEs
    Run 1 => Stat 2000 files [6,000 AFP ops]                                       233 ms
             IO Operations; afpd: 6000 READs, 6000 WRITEs | cnid_dbd: 0 READs, 0 WRITEs
    Run 1 => Enumerate dir with 2000 files [~51 AFP ops]                             3 ms
             IO Operations; afpd: 49 READs, 49 WRITEs | cnid_dbd: 0 READs, 0 WRITEs
    Run 1 => Lock then unlock 2000 open forks [4,000 AFP ops]                      155 ms
             IO Operations; afpd: 4000 READs, 4000 WRITEs | cnid_dbd: 0 READs, 0 WRITEs
    Run 1 => Deleting 2000 files [2,000 AFP ops]                                   440 ms
             IO Operations; afpd: 2000 READs, 8000 WRITEs | cnid_dbd: 7 READs, 6186 WRITEs
    Run 1 => Byte-range lock/unlock 2000 ranges in one fork [4,000 AFP ops]        157 ms
             IO Operations; afpd: 4000 READs, 4000 WRITEs | cnid_dbd: 0 READs, 0 WRITEs
    Run 1 => Create directory tree with 1000 dirs [1,110 AFP ops]                  223 ms
             IO Operations; afpd: 1110 READs, 3330 WRITEs | cnid_dbd: 4 READs, 2350 WRITEs
    Run 1 => Directory cache hits (20 dirs x 100 files) [2,020 AFP ops]             70 ms
             IO Operations; afpd: 2020 READs, 2020 WRITEs | cnid_dbd: 0 READs, 0 WRITEs
    Run 1 => Mixed cache operations (create/stat/enum/delete) on 1000 files [4,    382 ms
             IO Operations; afpd: 4100 READs, 8100 WRITEs | cnid_dbd: 4 READs, 5037 WRITEs
    Run 1 => Deep path traversal (20 levels x 100 walks) [2,000 AFP ops]            72 ms
             IO Operations; afpd: 2000 READs, 2000 WRITEs | cnid_dbd: 0 READs, 0 WRITEs
    Run 1 => Cache validation (2000 files x 3 lookups) [6,000 AFP ops]             232 ms
             IO Operations; afpd: 6000 READs, 6000 WRITEs | cnid_dbd: 0 READs, 0 WRITEs
    Successfully deleted test directory 'LanTest-40'

    Netatalk Lantest Results (Averages and standard deviations (±) for all tests, across 2 iterations (default))
    ============================================================================================================

    Test                                                                Time_ms  Time± AFPD_R AFPD_R± AFPD_W AFPD_W± CNID_R CNID_R± CNID_W CNID_W±   MB/s
    ------------------------------------------------------------------ -------- ------ ------ ------- ------ ------- ------ ------- ------ ------- ------
    Writing one large file [103 AFP ops]                                     40    2.2    101     0.0    300     1.0      0     0.0      0     0.0   2500
    Reading one large file [102 AFP ops]                                     17    2.2    273     5.0    172     5.0      0     0.0      0     0.0   5882
    Creating 2000 files [4,000 AFP ops]                                     407   51.6   4000     0.0   6000     0.0      4     0.0   4138    18.4      0
    Open, write 1024 bytes, close 2000 files [6,000 AFP ops]                261    7.8   6000     0.0   8000     0.0      0     0.0      0     0.0      0
    Open, read 512 bytes, close 2000 files [6,000 AFP ops]                  260    2.8   8000     0.0   4000     0.0      0     0.0      0     0.0      0
    Copying 2000 files client-side [14,000 AFP ops]                         673   75.0  16000     0.0  16000     0.0      3     1.4   4127    37.5      0
    Copying 2000 files server-side [2,000 AFP ops]                          448   77.8   6000     0.0   8000     0.0      4     0.0   4137     7.8      0
    Stat 2000 files [6,000 AFP ops]                                         232    1.0   6000     0.0   6000     0.0      0     0.0      0     0.0      0
    Enumerate dir with 2000 files [~51 AFP ops]                               3    1.0     49     0.0     49     0.0      0     0.0      0     0.0      0
    Lock then unlock 2000 open forks [4,000 AFP ops]                        153    2.2   4000     0.0   4000     0.0      0     0.0      0     0.0      0
    Deleting 2000 files [2,000 AFP ops]                                     416   33.2   2000     0.0   8000     0.0      5     2.2   6187     1.4      0
    Byte-range lock/unlock 2000 ranges in one fork [4,000 AFP ops]          155    2.2   4000     0.0   4000     0.0      0     0.0      0     0.0      0
    Create directory tree with 1000 dirs [1,110 AFP ops]                    208   21.2   1110     0.0   3330     0.0      3     1.4   2305    62.9      0
    Directory cache hits (20 dirs x 100 files) [2,020 AFP ops]               70    1.0   2020     0.0   2020     0.0      0     0.0      0     0.0      0
    Mixed cache operations (create/stat/enum/delete) on 1000 files [4,100 AFP ops]      381    1.4   4100     0.0   8100     0.0      4     0.0   5045    11.3      0
    Deep path traversal (20 levels x 100 walks) [2,000 AFP ops]              72    0.0   2000     0.0   2000     0.0      0     0.0      0     0.0      0
    Cache validation (2000 files x 3 lookups) [6,000 AFP ops]               230    2.8   6000     0.0   6000     0.0      0     0.0      0     0.0      0
    ------------------------------------------------------------------ -------- ------ ------ ------- ------ ------- ------ ------- ------ ------- ------
    Sum of all AFP OPs = 63486                                             4026         71653          85971             23          25939               

    Aggregates Summary:
    ------------------------------------------------------------------
    Average Time per AFP OP: 0.063 ms (from per-test medians)
    Average AFPD Reads per AFP OP: 1.129
    Average AFPD Writes per AFP OP: 1.354
    See afp_lantest manpage for more information: https://netatalk.io/manual/en/afp_lantest.1

    Dircache Statistics (/var/log/afpd.log):
    ------------------------------------------------------------------
    Jul 12 06:42:28.768144 afpd[41] {dircache.c:2074} (info:AFPDaemon): dircache statistics (ARC): (user: test) entries: 0, ghost_entries: 0, max_entries: 4001 (750 KB), config_max: 65536, lookups: 310256, hits: 281770 (90.8%), ghost_hits: 0 (0.0%), total_hits: (90.8%), misses: 28486 (9.2%), validations: 2819 (1.0%), added: 20305, removed: 20305, expunged: 181, invalid_on_use: 181, evicted: 0, validation_freq: 100

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

- Tests that involve large file operations (tests 1 and 2) will create temporary files in the
  test volume
- The **-g** and **-G** options significantly increase test file sizes and test duration
- Cache-focused tests (14-17) provide the most insight into netatalk's directory cache performance
- Multiple iterations (**-n**) are recommended for consistent performance measurements
- The tool requires write access to the specified AFP volume

# See Also

**afp_speedtest**(1), **afpd**(8), **netatalk**(8)
