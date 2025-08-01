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

**-h** *host*
: Server hostname or IP address (default: localhost)

**-p** *port*
: Server port number (default: 548)

**-s** *volume*
: Volume name to mount for testing

**-u** *user*
: Username for authentication (default: current uid)

**-w** *password*
: Password for authentication

**-n** *iterations*
: Number of test iterations to run (default: 1)

**-f** *tests*
: Specific tests to run, specified as digits (e.g., "134" runs tests 1, 3, and 4)

**-F** *bigfile*
: Use existing file in volume root for read test (file size must match -g/-G options)

**-g**
: Optimize for gigabit networks (increases file test size to 1 GB)

**-G**
: Optimize for 10-gigabit networks (increases file test size to 10 GB)

**-C**
: Run cache-focused tests only (tests 8-11)

**-v**
: Verbose output

**-V**
: Very verbose output

**-b**
: Debug mode

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

**(7) Create directory tree with 10^3 dirs**
: Tests nested directory creation

**(8) Directory cache hits [CACHE]**
: Tests directory and file lookup performance (1000 dirs + 10000 files)

**(9) Mixed cache operations [CACHE]**
: Tests mixed create/stat/enum/delete operations

**(10) Deep path traversal [CACHE]**
: Tests performance with deep directory structures

**(11) Cache validation [CACHE]**
: Tests directory cache validation mechanisms

## Cache-Focused Tests

Tests 8-11 are specifically designed to highlight directory cache performance improvements in
netatalk. These tests benefit significantly from optimized cache validation and probabilistic
validation features. Use the **-C** option to run only these cache-focused tests.

# Examples

Run all tests with default settings:

```bash
afp_lantest -h server.example.com -u testuser -w password -s TestVolume
```

Run only cache-focused tests:

```bash
afp_lantest -C -h server.example.com -u testuser -w password -s TestVolume
```

Run specific tests (file operations):

```bash
afp_lantest -f 123 -h server.example.com -u testuser -w password -s TestVolume
```

Run tests optimized for gigabit network:

```bash
afp_lantest -g -h server.example.com -u testuser -w password -s TestVolume
```

Run multiple iterations for statistical analysis:

```bash
afp_lantest -n 5 -h server.example.com -u testuser -w password -s TestVolume
```

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

**afptest**(1), **afpd**(8), **netatalk**(8)
