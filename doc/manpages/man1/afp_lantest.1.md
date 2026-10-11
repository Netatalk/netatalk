# Name

afp_lantest — AFP LAN performance and directory cache testing tool

# Synopsis

**afp_lantest** [-34567bcGgKVv] [-A *uam*] [-h *host*] [-p *port*] [-s *volume*] [-u *user*] [-w *password*]
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

**-A** *uam*
: Select authentication with the specified UAM name or alias.
  Use *clrtxt* for the legacy ClearTxt login path, *dhx* for DHCAST128,
  *dhx2* for DHX2, and *srp* for SRP.

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
: Run cache-focused tests only (tests 14-18)

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

By default, the test runner uses ClearTxt; **-A clrtxt** selects that same
legacy login path explicitly. Pass **-A dhx** or **-A dhx2** to use the
corresponding native encrypted UAM. Configure the selected UAM in
netatalk's afp.conf:

    [Global]
    uam list = uams_dhx.so uams_dhx2.so uams_clrtxt.so

# Available Tests

The following tests are available:

Most tests operate on a shared set of 2000 items, normalized to 2000 user
actions per test so their timings are comparable. Tests 3 and 5-12 form a
pipeline over the same 2000 files (create, write, read, copy, server-copy,
stat, enumerate, lock, delete); selecting any of them implies test 3. Test 4
creates a directory tree of the same size, so the two create workloads read
side by side.

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

**(4) Create 2000 dirs tree (20×9×10)**
: Tests nested directory creation (20 × 9 × 10 levels = 2000 dirs, aligning
with the other 2000-action tests). Timed: FPCreateDir per directory
[2,000 AFP ops]. Untimed cleanup: FPDelete per directory.

**(5) Open, write 1024 bytes, close 2000 files**
: Tests the full small-write lifecycle per file (implies test 3). Timed, per
file: FPOpenFork + FPWrite + FPCloseFork [3 x 2000 = 6,000 AFP ops].

**(6) Open, read 1024 bytes, close 2000 files**
: Tests the full small-read lifecycle a real client pays per file, including
the lock management carried by each open and close (implies tests 3, 4).
Mirrors test 5's timed open+write+close — reading back the full 1024 bytes
test 5 wrote — so the read and write results are directly comparable. Timed,
per file: FPOpenFork + FPRead + FPCloseFork [3 x 2000 = 6,000 AFP ops].

**(7) Copying 1000 files client-side (R+W)**
: Tests copying where the client reads each file's data and writes it back to
a new file, as file managers without server-side copy do (implies tests 3, 4).
1000 copies = 1000 reads + 1000 writes, so the total aligns with the other
2000-action tests. Timed, per file: FPOpenFork + FPRead + FPCloseFork on
the source, then FPCreateFile + FPOpenFork + FPWrite + FPCloseFork on the copy
[7 x 1000 = 7,000 AFP ops]. Untimed cleanup: FPDelete per copy.

**(8) Copying 2000 files server-side**
: Tests FPCopyFile, where the data never crosses the wire (implies tests 3, 4).
Timed, per file: FPCopyFile [1 x 2000 = 2,000 AFP ops]. Untimed cleanup:
FPDelete per copy. Contrast with test 7 to see the round-trip savings.

**(9) Stat (lookup+getparams) 2000 files**
: Tests file lookup and metadata query performance (implies test 3). Timed,
per file: a minimal lookup probe plus one full-bitmap FPGetFileDirParms
request [2 x 2000 = 4,000 AFP ops].

**(10) Enumerate dir with 2000 files**
: Tests directory enumeration with many files (implies test 3). Timed:
FPEnumerateExt2 in 40-entry chunks over the 2000-file directory
[~51 AFP ops].

**(11) Lock then unlock 2000 open forks**
: Tests refnum lookup and lock setup against a full open-fork table (implies
test 3). Timed: FPByteRangeLock lock + unlock on each fork
[2 x 2000 = 4,000 AFP ops]. Untimed: FPOpenFork x2000 setup,
FPCloseFork x2000 cleanup.

**(12) Deleting 2000 files**
: Tests file deletion performance (implies test 3). Timed, per file: FPDelete
[1 x 2000 = 2,000 AFP ops].

**(13) Byte-range lock/unlock 2000 ranges in one fork**
: Tests per-fork byte-range lock tracking with many locks held at once. Timed:
FPByteRangeLock lock x2000 then unlock x2000 on distinct ranges in a single
fork [4,000 AFP ops]. Untimed: file create/write/open setup and cleanup.

**(14) Directory cache hits [CACHE]**
: Tests directory and file lookup performance (20 dirs x 100 files). Timed:
FPGetFileDirParms per directory and per file [2,020 AFP ops]. Untimed:
FPCreateDir/FPCreateFile setup, FPDelete cleanup.

**(15) Mixed cache operations [CACHE]**
: Tests the cache lifecycle over 500 files, 4 user actions each
(create/stat/stat/delete = 2000 actions; self-contained). Timed, per file:
FPCreateFile + FPGetFileDirParms + FPGetFileDirParms + FPDelete, plus
FPEnumerate every 10th file [4 x 500 + 50 = 2,050 AFP ops].

**(16) Deep path traversal [CACHE]**
: Tests 100 walks of a 20-level deep directory tree. Timed: FPGetFileDirParms
per level per walk [20 x 100 = 2,000 AFP ops]. Untimed: FPCreateDir setup,
FPDelete cleanup.

**(17) Cache validation [CACHE]**
: Tests hot-cache revalidation over 500 files, 4 repeated lookups each
(2000 user actions). Timed, per file: four FPGetFileDirParms metadata
lookups [4 x 500 = 2,000 AFP ops]. Untimed: FPCreateFile setup, FPDelete
cleanup.

**(18) Folder item counts after listing each folder [CACHE]**
: Tests the folder item counts Finder asks for when a folder is selected in
column view: each of 20 folders of 50 files is listed, then their parent is
listed with Finder's directory bitmap, for 20 rounds. Timed: FPEnumerate of
the folder + FPEnumerate of the parent per folder per round
[2 x 20 x 20 = 800 AFP ops]. Untimed: FPCreateDir/FPCreateFile setup, a
two-second wait, FPDelete cleanup.

## Cache-Focused Tests

Tests 14-18 are specifically designed to highlight directory cache performance improvements in
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
    Found privilege-dropped afpd process for user 'test': PID 67
    IO monitoring enabled (afpd: 67)

    Run 1 => Writing one large file [103 AFP ops]                                   61 ms for 100 MiB (avg. 1718 MB/s)
             IO Operations; afpd: 98 READs, 292 WRITEs
    Run 1 => Reading one large file [102 AFP ops]                                   24 ms for 100 MiB (avg. 4369 MB/s)
             IO Operations; afpd: 287 READs, 189 WRITEs
    Run 1 => Creating 2000 files [4,000 AFP ops]                                   557 ms
             IO Operations; afpd: 4120 READs, 21202 WRITEs
    Run 1 => Create 2000 dirs tree (20×9×10) [2,000 AFP ops]                     372 ms
             IO Operations; afpd: 2128 READs, 20886 WRITEs
    Run 1 => Open, write 1024 bytes, close 2000 files [6,000 AFP ops]              637 ms
             IO Operations; afpd: 6000 READs, 8000 WRITEs
    Run 1 => Open, read 1024 bytes, close 2000 files [6,000 AFP ops]               612 ms
             IO Operations; afpd: 8000 READs, 4000 WRITEs
    Run 1 => Copying 1000 files client-side (R+W) [7,000 AFP ops]                  824 ms
             IO Operations; afpd: 8066 READs, 15484 WRITEs
    Run 1 => Copying 2000 files server-side [2,000 AFP ops]                        378 ms
             IO Operations; afpd: 6137 READs, 21325 WRITEs
    Run 1 => Stat (lookup+getparams) 2000 files [4,000 AFP ops]                    391 ms
             IO Operations; afpd: 4000 READs, 4000 WRITEs
    Run 1 => Enumerate dir with 2000 files [~51 AFP ops]                             8 ms
             IO Operations; afpd: 49 READs, 49 WRITEs
    Run 1 => Lock then unlock 2000 open forks [4,000 AFP ops]                      314 ms
             IO Operations; afpd: 4000 READs, 4000 WRITEs
    Run 1 => Deleting 2000 files [2,000 AFP ops]                                   372 ms
             IO Operations; afpd: 2112 READs, 16554 WRITEs
    Run 1 => Byte-range lock/unlock 2000 ranges in one fork [4,000 AFP ops]        425 ms
             IO Operations; afpd: 4000 READs, 4000 WRITEs
    Run 1 => Directory cache hits (20 dirs x 100 files) [2,020 AFP ops]            184 ms
             IO Operations; afpd: 2020 READs, 2020 WRITEs
    Run 1 => Mixed cache operations (create/stat/enum/delete) on 500 files [2,0    256 ms
             IO Operations; afpd: 2062 READs, 9575 WRITEs
    Run 1 => Deep path traversal (20 levels x 100 walks) [2,000 AFP ops]           168 ms
             IO Operations; afpd: 2000 READs, 2000 WRITEs
    Run 1 => Cache validation (500 files x 4 lookups) [2,000 AFP ops]              162 ms
             IO Operations; afpd: 2000 READs, 2000 WRITEs
    Run 1 => Folder item counts after listing each folder (20 x 50 files) [800      77 ms
             IO Operations; afpd: 800 READs, 800 WRITEs
    Successfully deleted test directory 'LanTest-66'

    Netatalk Lantest Results (Averages and standard deviations (±) for all tests, across 2 iterations (default))
    ============================================================================================================

    Test                                                                Time_ms  Time± AFPD_R AFPD_R± AFPD_W AFPD_W±   MB/s
    ------------------------------------------------------------------ -------- ------ ------ ------- ------ ------- ------
    Writing one large file [103 AFP ops]                                     71   14.9     98     0.0    292     0.0   1408
    Reading one large file [102 AFP ops]                                     24    0.0    287     0.0    189     0.0   4166
    Creating 2000 files [4,000 AFP ops]                                     551    7.8   4127     9.9  21257    78.5      0
    Create 2000 dirs tree (20×9×10) [2,000 AFP ops]                       394   31.1   2132     6.4  20914    40.3      0
    Open, write 1024 bytes, close 2000 files [6,000 AFP ops]                525  158.4   6000     0.0   8000     0.0      0
    Open, read 1024 bytes, close 2000 files [6,000 AFP ops]                 489  173.2   8000     0.0   4000     0.0      0
    Copying 1000 files client-side (R+W) [7,000 AFP ops]                    696  181.0   8074    11.3  15493    13.5      0
    Copying 2000 files server-side [2,000 AFP ops]                          321   80.6   6137     1.0  21329     6.4      0
    Stat (lookup+getparams) 2000 files [4,000 AFP ops]                      332   82.7   4000     0.0   4000     0.0      0
    Enumerate dir with 2000 files [~51 AFP ops]                               6    2.8     49     0.0     49     0.0      0
    Lock then unlock 2000 open forks [4,000 AFP ops]                        296   25.5   4000     0.0   4000     0.0      0
    Deleting 2000 files [2,000 AFP ops]                                     314   82.0   2122    14.9  16565    15.6      0
    Byte-range lock/unlock 2000 ranges in one fork [4,000 AFP ops]          314  156.3   4000     0.0   4000     0.0      0
    Directory cache hits (20 dirs x 100 files) [2,020 AFP ops]              177    9.2   2020     0.0   2020     0.0      0
    Mixed cache operations (create/stat/enum/delete) on 500 files [2,050 AFP ops]      237   26.9   2062     0.0   9570     7.1      0
    Deep path traversal (20 levels x 100 walks) [2,000 AFP ops]             159   12.0   2000     0.0   2000     0.0      0
    Cache validation (500 files x 4 lookups) [2,000 AFP ops]                145   24.0   2000     0.0   2000     0.0      0
    Folder item counts after listing each folder (20 x 50 files) [800 AFP ops]       79    3.6    800     0.0    800     0.0      0
    ------------------------------------------------------------------ -------- ------ ------ ------- ------ ------- ------
    Sum of all AFP OPs = 50126                                             5130         57908         136478               

    Aggregates Summary:
    ------------------------------------------------------------------
    Average Time per AFP OP: 102 µs (from per-test medians)
    Average AFPD Reads per AFP OP: 1.155
    Average AFPD Writes per AFP OP: 2.723
    See afp_lantest manpage for more information: https://netatalk.io/manual/en/afp_lantest.1

    Dircache Statistics (/var/log/afpd.log):
    ------------------------------------------------------------------
    Oct 07 10:13:42.536463 afpd[67] {dircache.c:2437} (info:AFPDaemon): dircache statistics (ARC): (user: test) entries: 0, ghost_entries: 0, max_entries: 4001 (812 KB), config_max_entries: 65536, lookups: 292268, hits: 265961 (91.0%), ghost_hits: 0 (0.0%), total_hits: (91.0%), misses: 26307 (9.0%), validations: 2661 (1.0%), added: 18167, removed: 18167, expunged: 140, invalid_on_use: 140, evicted: 0, covered_cancelled: 4068, validation_freq: 100
    Oct 07 10:13:42.536467 afpd[67] {dircache.c:2467} (info:AFPDaemon): ARC ghost performance: (user: test) ghost_hits: 0 (0.0%), ghost_hits(B1=0, B2=0), learning_benefit: (0.0%)
    Oct 07 10:13:42.536471 afpd[67] {dircache.c:2476} (info:AFPDaemon): ARC table state: (user: test) T1=0 (recent/cached), T2=0 (frequent/cached), B1=0 (recent/ghost), B2=0 (frequent/ghost), total_cached=0/65536, total_ghosts=0, freq_bias: (0.0%)
    Oct 07 10:13:42.536475 afpd[67] {dircache.c:2486} (info:AFPDaemon): ARC adaptation: (user: test) p=0/65536 (0.0% target for T1), p_range=[65536,0], adaptations=0 (increases=0, decreases=0)
    Oct 07 10:13:42.536478 afpd[67] {dircache.c:2497} (info:AFPDaemon): ARC operations: (user: test) hits(T1=18167, T2=254159), promotions(T1→T2=18167), evictions(T1=0, T2=0)
    Oct 07 10:13:42.536482 afpd[67] {dircache.c:2537} (info:AFPDaemon): dircache statistics (hints): (user: test) sent: 68522, dropped: 0, received: 0, acted_on: 0, no_match: 0
    Oct 07 10:13:42.536486 afpd[67] {dircache.c:2554} (info:AFPDaemon): dircache statistics (AD): (user: test) hits: 76770, misses: 4265, no_ad: 0, hit_ratio: 94.7%
    Oct 07 10:13:42.536490 afpd[67] {dircache.c:2560} (info:AFPDaemon): dircache statistics (offcnt): (user: test) requests: 16000, cached: 15959, walked: 41 (first: 38, older: 0, confirmed: 3, changed: 0), failed: 0, entries: 2091

## IO Monitoring Result Columns

    Time(ms) = Test runtime in milliseconds
    Time±    = Test runtime standard deviation
    AFPD_R   = afpd process IO Read operations
    AFPD_R±  = afpd process IO Read operation standard deviation
    AFPD_W   = afpd process IO Write operations
    AFPD_W±  = afpd process IO Write operation standard deviation

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
- Cache-focused tests (14-18) provide the most insight into netatalk's directory cache performance
- Multiple iterations (**-n**) are recommended for consistent performance measurements
- The tool requires write access to the specified AFP volume

# See Also

**afp_speedtest**(1), **afpd**(8), **netatalk**(8)
