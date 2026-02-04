# Name

afp_speedtest — Simple AFP file transfer benchmarking tool

# Synopsis

**afp_speedtest** [-1234567acDeiLTVvy] [-h *host*] [-p *port*] [-s *volume*] [-P *path*] [-S *volume2*]
[-u *user*] [-w *password*] [-n *iterations*] [-W *warmup*] [-t *delay*] [-d *size*] [-z *sizes*]
[-q *quantum*] [-r *requests*] [-F *file*] [-f *test*]

# Description

**afp_speedtest** is an AFP benchmark testsuite for read, write, copy and server-side copy operations.
It can operate in two modes:

- **AFP Mode** (default): Tests AFP protocol performance over the network
- **Local Mode** (`-L`): Direct filesystem I/O baseline benchmarking using POSIX syscalls

The tool supports comprehensive performance analysis including:

- Multiple iteration testing with warmup runs
- Statistical analysis (mean, median, standard deviation, percentiles)
- File size sweeping to test performance across different file sizes
- TCP network metrics tracking (AFP mode only)
- CSV output format for data analysis

# Options

**-1**
: Use AFP 2.1 protocol version

**-2**
: Use AFP 2.2 protocol version

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

**-a**
: Don't flush to disk after write

**-c**
: CSV output mode (automatically enables statistics and quiet mode)

**-d** *size*
: File size in Mbytes (default: 64 MB, or default size sweep if not specified)

**-D**
: Disable O_DIRECT in Local mode (auto-enabled by default for realistic AFP comparison)
: Use this flag to test buffered I/O performance with kernel page cache
: Ignored in AFP mode (O_DIRECT not applicable)

**-e**
: Use sparse file

**-f** *tests*
: Comma-separated list of tests to run: Read, Write, Copy, ServerCopy (default: Write)
: Note: ServerCopy is automatically skipped in Local mode as it requires an AFP server

**-F** *file*
: Read from existing file in volume root folder (default: creates temporary files)

**-h** *host*
: Server hostname or IP address (default: localhost, ignored in Local mode)

**-i**
: Interactive mode – prompt user before each test (used for debugging)

**-L**
: Local mode – use POSIX calls for direct filesystem I/O instead of AFP protocol
: Must be used with `-P` to specify directory path
: Provides baseline performance measurements without network/protocol overhead

**-n** *iterations*
: Number of test iterations to run (default: 1)
: When > 1, automatically enables statistics output

**-p** *port*
: Server port number (default: 548, ignored in Local mode)

**-P** *path*
: Local directory path for Local mode testing (required when using `-L`)

**-q** *size*
: Packet/buffer size in Kbytes (default: detected server quantum in AFP mode, 1024 KB in Local mode)

**-r** *number*
: Number of outstanding pipelined requests for parallel I/O operations (default: 1)
: Higher values allow multiple in-flight AFP requests, improving throughput on high-latency connections
: Note: Values > 1 require careful tuning and may not benefit all scenarios

**-s** *volume*
: Volume name to mount for testing (AFP mode only, ignored in Local mode)

**-S** *volume*
: Second volume name for cross-volume copy testing (AFP mode only)

**-t** *seconds*
: Delay in seconds between test iterations (default: 0)
: Useful for cooling down between iterations or simulating real-world usage patterns

**-T**
: Show statistics (mean, median, std dev, min, max, P95)
: Automatically enabled when iterations > 1 or when using size sweep

**-u** *user*
: Username for AFP server authentication (default: current uid, ignored in Local mode)

**-v**
: Verbose output

**-V**
: Very verbose output

**-w** *password*
: Password for AFP server authentication (ignored in Local mode)

**-W** *warmup*
: Number of warmup runs excluded from statistics (default: 1)
: Warmup runs are marked with [W#] prefix and not included in statistics

**-y**
: Use a new file for each run (default: reuse same filename)

**-z** *sizes*
: File size sweep mode – comma-separated list of sizes in MB
: Example: `-z 0.004,0.008,0.016,0.032,0.064,0.128,0.256,0.512,1,2,4,8,16,32,64,128,256,512`
: Minimum: 0.004 MB (4 KB), Maximum: 1024 MB
: If neither `-d` nor `-z` is specified, uses default 18-size sweep: 4KB,8KB,16KB,32KB,64KB,128KB,256KB,512KB,1MB,2MB,4MB,8MB,16MB,32MB,64MB,128MB,256MB,512MB

# Configuration

The test runner AFP client only supports the ClearTxt UAM currently.
Configure the UAM in netatalk's afp.conf:

    [Global]
    uam list = uams_clrtxt.so

# Output Modes

## Text Mode (Default)

Displays formatted tables with:

- Per-iteration timing results with throughput
- Statistical summaries (when enabled)
- Network/TCP metrics comparison tables (AFP mode only)
- File size performance summary table (size sweep mode)
- TCP metrics evolution table (AFP mode with size sweep)

## CSV Mode (`-c`)

Generates three CSV tables for data analysis:

1. **Test Data Table**: Individual iteration results (test_name, iteration, file_size_mb, microseconds, throughput_mbs)
2. **File Size Summary Table**: Aggregated statistics per file size (mean, median, min, max throughput and timing)
3. **TCP Metrics Evolution Table** (AFP mode only): Initial vs final TCP metrics across all tests

# Examples

## AFP Mode Examples

Run 5 iterations of all four benchmark tests over AFP:

    afp_speedtest -h 10.0.0.10 -s speed1 -u myuser -w mypass -n 5 -f Read,Write,Copy,ServerCopy

Run a single 64 MB write test:

    afp_speedtest -h 10.0.0.10 -s speed1 -u myuser -w mypass -d 64 -f Write

Run file size sweep with 10 iterations per size and export to CSV:

    afp_speedtest -h 10.0.0.10 -s speed1 -u myuser -w mypass -n 10 -c -f Read,Write > results.csv

## Local Mode Examples

Baseline filesystem performance with 5 iterations:

    afp_speedtest -L -P /mnt/fast_disk -n 5 -f Read,Write,Copy

Local mode with custom file size sweep:

    afp_speedtest -L -P /tmp/test -z 1,4,16,64,256 -n 3 -f Read,Write

Local mode with buffered I/O (O_DIRECT disabled) and CSV output:

    afp_speedtest -L -D -P /mnt/nvme -c -n 10 -f Read,Write > nvme_buffered.csv

Local mode with O_DIRECT (default, for AFP comparison):

    afp_speedtest -L -P /mnt/nvme -c -n 10 -f Read,Write > nvme_direct.csv

# Output Example

    AFP Speedtest - Configuration
    ════════════════════════════════════════
     Mode:            Local (Direct Filesystem I/O)
     AFP Version:     N/A (Local)
     Directory:       /tmp/speedtest
     Tests:           Read,Write,Copy
     File Size:       64 MB
     Iterations:      5
     Warmup Runs:     1
     Statistics:      Enabled
    
    
    ===== Test Passes for Read =====
    Read quantum 1024 KB, size 64 MB
    Warmup: 1 runs, Measured: 5 runs
    run	  microsec	  MB/s
    [W1]	    11739	5451.91
    1	     9511	6729.05
    2	     9700	6597.94
    3	     9174	6976.24
    4	     8949	7151.64
    5	     9050	7071.82
    
    ===== Statistics for Read =====
    File Size:      64 MB
    Iterations:     5
    Mean:           9277 μs (6898.93 MB/s)
    Median:         9174 μs (6976.24 MB/s)
    Std Dev:        284 μs (3.06%)
    Min:            8949 μs (7151.64 MB/s)
    Max:            9700 μs (6597.94 MB/s)

# See Also

**afp_lantest**(1), **afpd**(8)
