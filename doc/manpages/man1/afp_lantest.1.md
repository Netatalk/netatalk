# Name

afp_lantest — AFP LAN performance and directory cache testing tool

# Synopsis

**afp_lantest** [-34567GgVvbC] [-h *host*] [-p *port*] [-s *volume*] [-u *user*] [-w *password*]
[-n *iterations*] [-f *tests*] [-F *bigfile*]

# Description

**afp_lantest** is a comprehensive AFP (Apple Filing Protocol) performance testing tool designed to
benchmark various aspects of AFP servers, with special focus on directory cache performance
improvements in netatalk.

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

**-C**
: Run cache-focused tests only (tests 9-12)

**-f** *tests*
: Specific tests to run, specified as digits (e.g., "134" runs tests 1, 3, and 4)

**-F** *bigfile*
: Use existing file in volume root for read test (file size must match -g/-G options)

**-g**
: Optimize for gigabit networks (increases file test size to 1 GB)

**-G**
: Optimize for 10-gigabit networks (increases file test size to 10 GB)

**-h** *host*
: Server hostname or IP address (default: localhost)

**-n** *iterations*
: Number of test iterations to run (default: 1)

**-p** *port*
: Server port number (default: 548)

**-s** *volume*
: Volume name to mount for testing

**-u** *user*
: Username for authentication (default: current uid)

**-v**
: Verbose output

**-V**
: Very verbose output

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
validation features. Use the **-C** option to run only these cache-focused tests.

# Examples

Run all tests with default settings:

    afp_lantest -h server.example.com -u testuser -w password -s TestVolume

Run only cache-focused tests:

    afp_lantest -C -h server.example.com -u testuser -w password -s TestVolume

Run specific tests (file operations):

    afp_lantest -f 123 -h server.example.com -u testuser -w password -s TestVolume

Run tests optimized for gigabit network:

    afp_lantest -g -h server.example.com -u testuser -w password -s TestVolume

Run multiple iterations for statistical analysis:

    afp_lantest -n 5 -h server.example.com -u testuser -w password -s TestVolume

Run the afp_lantest benchmark using AFP 3.4.

    % afp_lantest -7 -h localhost -p 548 -u test -w test -s "File Sharing" -n 2
    Run 0 => Opening, stating and reading 512 bytes from 1000 files   3237 ms
    Run 0 => Writing one large file                                    146 ms for 100 MB (avg. 718 MB/s)
    Run 0 => Reading one large file                                     36 ms for 100 MB (avg. 2912 MB/s)
    Run 0 => Locking/Unlocking 10000 times each                        772 ms
    Run 0 => Creating dir with 2000 files                             3615 ms
    Run 0 => Enumerate dir with 2000 files                             755 ms
    Run 0 => Deleting dir with 2000 files                             1245 ms
    Run 0 => Create directory tree with 10^3 dirs                     1724 ms
    Run 0 => Directory cache hits (1000 dir + 10000 file lookups)     3056 ms
    Run 0 => Mixed cache operations (create/stat/enum/delete)          484 ms
    Run 0 => Deep path traversal (nested directory navigation)         377 ms
    Run 0 => Cache validation efficiency (metadata changes)           8822 ms
    Run 1 => Opening, stating and reading 512 bytes from 1000 files   3448 ms
    Run 1 => Writing one large file                                     79 ms for 100 MB (avg. 1327 MB/s)
    Run 1 => Reading one large file                                     37 ms for 100 MB (avg. 2833 MB/s)
    Run 1 => Locking/Unlocking 10000 times each                        779 ms
    Run 1 => Creating dir with 2000 files                             3731 ms
    Run 1 => Enumerate dir with 2000 files                             587 ms
    Run 1 => Deleting dir with 2000 files                             1156 ms
    Run 1 => Create directory tree with 10^3 dirs                     1802 ms
    Run 1 => Directory cache hits (1000 dir + 10000 file lookups)     3006 ms
    Run 1 => Mixed cache operations (create/stat/enum/delete)          463 ms
    Run 1 => Deep path traversal (nested directory navigation)         247 ms
    Run 1 => Cache validation efficiency (metadata changes)           8565 ms

    Netatalk Lantest Results (average times across 2 iterations)
    ===================================

    Test 1: Opening, stating and reading 512 bytes from 1000 files
     Average:   3342 ms ± 149.2 ms (std dev)

    Test 2: Writing one large file
     Average:    112 ms ± 47.4 ms (std dev)
     Throughput: 936 MB/s (Write, 100 MB file)

    Test 3: Reading one large file
     Average:     36 ms ± 1.0 ms (std dev)
     Throughput: 2912 MB/s (Read, 100 MB file)

    Test 4: Locking/Unlocking 10000 times each
     Average:    775 ms ± 5.0 ms (std dev)

    Test 5: Creating dir with 2000 files
     Average:   3673 ms ± 82.0 ms (std dev)

    Test 6: Enumerate dir with 2000 files
     Average:    671 ms ± 118.8 ms (std dev)

    Test 7: Deleting dir with 2000 files
     Average:   1200 ms ± 62.9 ms (std dev)

    Test 8: Create directory tree with 1000 dirs
     Average:   1763 ms ± 55.2 ms (std dev)

    Test 9: Directory cache hits (1000 dir + 10000 file lookups)
     Average:   3031 ms ± 35.4 ms (std dev)

    Test 10: Mixed cache operations (create/stat/enum/delete)
     Average:    473 ms ± 14.9 ms (std dev)

    Test 11: Deep path traversal (nested directory navigation)
     Average:    312 ms ± 91.9 ms (std dev)

    Test 12: Cache validation efficiency (metadata changes)
     Average:   8693 ms ± 181.7 ms (std dev)

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
