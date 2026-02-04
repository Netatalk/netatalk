/*
 * Copyright (c) 2026, Andy Lemin (andylemin)
 * Credits; Based on work by Netatalk contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <math.h>
#include <signal.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#include "afpclient.h"
#include "afpcmd.h"
#include "afphelper.h"
#include "testhelper.h"
#include "afp_tcp_analytics.h"
#include "speedtest_local_vfs.h"

/* For compiling on OS X */
#ifndef MAP_ANONYMOUS
#ifdef MAP_ANON
#define MAP_ANONYMOUS MAP_ANON
#endif
#endif

int32_t is_there(CONN *conn, uint16_t volume, int32_t did, char *name);

uint16_t VolID;
uint16_t VolID2;
static DSI *dsi;
CONN *Conn;

int ExitCode = 0;

DSI *Dsi;
char Data[30000];
char *Buffer;
struct timeval Timer_start;
struct timeval Timer_end;
#define KILOBYTE 1024
#define MEGABYTE (KILOBYTE*KILOBYTE)

/* Statistics calculation support */
#define MAX_ITERATIONS 1000

typedef struct {
    unsigned long long values[MAX_ITERATIONS];
    int count;
    double mean;
    double median;
    double stddev;
    unsigned long long min;
    unsigned long long max;
    double percentile_95;  /* Only shown when n >= 20 */
    double cv_percent;     /* coefficient of variation */
} TestStats;

/* Global statistics tracking */
static TestStats current_test_stats;
static int enable_statistics = 0;
static int in_warmup_phase = 0;

/* Output format control */
typedef enum {
    OUTPUT_TEXT,
    OUTPUT_CSV
} OutputFormat;

static OutputFormat output_format = OUTPUT_TEXT;
static int csv_header_printed = 0;

/* TCP Analytics session for network metrics tracking */
static TcpAnalyticsSession tcp_session;

/* ------------------------------- */
char    *Server = "localhost";
static int     Port = DSI_AFPOVERTCP_PORT;
static char    *Password = "";
char    *Vol = "";
char    *Vol2 = "";
static char    *LocalPath = "";  /* Local filesystem path for -L mode */
char    *User;
int     Version = 34;
char    *Test = "Write";
static char    *Filename;

char *vers = "AFP3.4";
char *uam = "Cleartxt Passwrd";

static int Count = 1;
static int WarmupRuns = 1;
static int DelaySeconds = 0;
static off_t Size = 64 * MEGABYTE;

/* File size sweeping support */
#define MAX_SIZE_SWEEP 32
static int size_sweep_enabled = 0;
static off_t size_sweep_values[MAX_SIZE_SWEEP];
static int size_sweep_count = 0;

typedef struct {
    double file_size_mb;  /* Use double to support fractional MB (e.g., 0.004 for 4KB) */
    double mean_throughput_mbs;
    double median_throughput_mbs;
    unsigned long long mean_time_us;
    unsigned long long median_time_us;
    unsigned long long min_time_us;
    unsigned long long max_time_us;
    double stddev_us;
} SizeSweepResult;

/* Per-test type result tracking for size sweep mode */
typedef struct {
    SizeSweepResult read_results[MAX_SIZE_SWEEP];
    SizeSweepResult write_results[MAX_SIZE_SWEEP];
    SizeSweepResult copy_results[MAX_SIZE_SWEEP];
    SizeSweepResult servercopy_results[MAX_SIZE_SWEEP];
    int read_count;
    int write_count;
    int copy_count;
    int servercopy_count;
} AllTestResults;

static AllTestResults all_results;

static size_t Quantum = 0;
static int Request = 1;
static int Delete = 0;
static int Sparse = 0;
static int Local = 0;
static int Flush = 1;
static int Direct = 1;

/* not used */
CONN *Conn2;
int  Mac = 0;
int EmptyVol = 0;
int PassCount = 0;
int FailCount = 0;
int SkipCount = 0;
int NotTestedCount = 0;
char FailedTests[1024][256] = {{0}};
char NotTestedTests[1024][256] = {{0}};
char SkippedTests[1024][256] = {{0}};

struct vfs VFS = {
    FPGetFileDirParams,
    FPCreateDir,
    FPCreateFile,
    FPOpenFork,
    FPWriteHeader,
    FPWriteFooter,
    FPFlushFork,
    FPCloseFork,
    FPDelete,
    FPSetForkParam,
    FPWrite,
    FPRead,
    FPReadHeader,
    FPReadFooter,
    FPCopyFile,
    FPOpenVol,
    FPCloseVol
};

/*!
 * @brief Display message and wait for user to press Enter
 */
static void press_enter(char *s)
{
    if (!Interactive) {
        return;
    }

    if (s) {
        fprintf(stdout, "--> Performing: %s\n", s);
    }

    fprintf(stdout, "Press <ENTER> to continue.\n");

    while (fgetc(stdin) != '\n')
        ;
}

/* ------------------ */
unsigned long long delta(void)
{
    unsigned long long s, e;
    s  = Timer_start.tv_sec;
    s *= 1000000;
    s += Timer_start.tv_usec;
    e  = Timer_end.tv_sec;
    e *= 1000000;
    e += Timer_end.tv_usec;
    return e - s;
}

/*!
 * @brief Compare function for qsort() to sort unsigned long long values
 */
static int compare_ulonglong(const void *a, const void *b)
{
    unsigned long long aa = *(const unsigned long long *)a;
    unsigned long long bb = *(const unsigned long long *)b;

    if (aa < bb) {
        return -1;
    }

    if (aa > bb) {
        return 1;
    }

    return 0;
}

/*!
 * @brief Calculate mean, median, stddev, and percentiles from timing samples
 */
static void calculate_statistics(TestStats *stats)
{
    if (stats->count == 0) {
        return;
    }

    /* Calculate mean */
    unsigned long long sum = 0;

    for (int i = 0; i < stats->count; i++) {
        sum += stats->values[i];
    }

    stats->mean = (double)sum / (double)stats->count;
    /* Sort for median and percentiles */
    unsigned long long sorted[MAX_ITERATIONS];
    memcpy(sorted, stats->values, stats->count * sizeof(unsigned long long));
    qsort(sorted, stats->count, sizeof(unsigned long long), compare_ulonglong);
    /* Min and max */
    stats->min = sorted[0];
    stats->max = sorted[stats->count - 1];

    /* Median */
    if (stats->count % 2 == 0) {
        stats->median = ((double)sorted[stats->count / 2 - 1] +
                         (double)sorted[stats->count / 2]) / 2.0;
    } else {
        stats->median = (double)sorted[stats->count / 2];
    }

    /* Percentiles */
    int p95_idx = (int)((stats->count - 1) * 0.95);
    stats->percentile_95 = (double)sorted[p95_idx];
    /* Standard deviation */
    double variance = 0;

    for (int i = 0; i < stats->count; i++) {
        double diff = (double)stats->values[i] - stats->mean;
        variance += diff * diff;
    }

    stats->stddev = sqrt(variance / (double)stats->count);

    /* Coefficient of variation */
    if (stats->mean > 0) {
        stats->cv_percent = (stats->stddev / stats->mean) * 100.0;
    } else {
        stats->cv_percent = 0;
    }
}

/*!
 * @brief Record single timing measurement for current test
 */
static void record_timing(unsigned long long microseconds)
{
    if (enable_statistics && !in_warmup_phase &&
            current_test_stats.count < MAX_ITERATIONS) {
        current_test_stats.values[current_test_stats.count++] = microseconds;
    }
}

/*!
 * @brief Reset statistics counters for next test iteration
 */
static void reset_statistics(void)
{
    memset(&current_test_stats, 0, sizeof(current_test_stats));
}

/*!
 * @brief Print statistics in human-readable text format
 */
static void print_statistics_text(const char *test_name)
{
    if (current_test_stats.count < 2) {
        return;  /* Need at least 2 samples for meaningful statistics */
    }

    calculate_statistics(&current_test_stats);
    fprintf(stdout, "\n===== Statistics for %s =====\n", test_name);

    if (Size < MEGABYTE) {
        fprintf(stdout, "File Size:      %ld KB\n", Size / KILOBYTE);
    } else {
        fprintf(stdout, "File Size:      %ld MB\n", Size / MEGABYTE);
    }

    fprintf(stdout, "Iterations:     %d\n", current_test_stats.count);

    /* MB/s = (bytes * 1,000,000 μs/s) / (microseconds * 1,048,576 bytes/MB) */
    /* Guard against division by zero for operations faster than timer resolution */
    if (current_test_stats.mean > 0) {
        fprintf(stdout, "Mean:           %.0f μs (%.2f MB/s)\n",
                current_test_stats.mean,
                ((double)Size * 1000000.0) / (MEGABYTE * current_test_stats.mean));
    } else {
        fprintf(stdout, "Mean:           <1 μs (too fast to measure)\n");
    }

    if (current_test_stats.median > 0) {
        fprintf(stdout, "Median:         %.0f μs (%.2f MB/s)\n",
                current_test_stats.median,
                ((double)Size * 1000000.0) / (MEGABYTE * current_test_stats.median));
    } else {
        fprintf(stdout, "Median:         <1 μs (too fast to measure)\n");
    }

    fprintf(stdout, "Std Dev:        %.0f μs (%.2f%%)\n",
            current_test_stats.stddev, current_test_stats.cv_percent);

    if (current_test_stats.min > 0) {
        fprintf(stdout, "Min:            %llu μs (%.2f MB/s)\n",
                current_test_stats.min,
                ((double)Size * 1000000.0) / (MEGABYTE * (double)current_test_stats.min));
    } else {
        fprintf(stdout, "Min:            <1 μs (too fast to measure)\n");
    }

    if (current_test_stats.max > 0) {
        fprintf(stdout, "Max:            %llu μs (%.2f MB/s)\n",
                current_test_stats.max,
                ((double)Size * 1000000.0) / (MEGABYTE * (double)current_test_stats.max));
    } else {
        fprintf(stdout, "Max:            <1 μs (too fast to measure)\n");
    }

    /* Only show P95 if we have enough samples for it to be meaningful */
    if (current_test_stats.count >= 20) {
        if (current_test_stats.percentile_95 > 0) {
            fprintf(stdout, "P95:            %.0f μs (%.2f MB/s)\n",
                    current_test_stats.percentile_95,
                    ((double)Size * 1000000.0) / (MEGABYTE * current_test_stats.percentile_95));
        } else {
            fprintf(stdout, "P95:            <1 μs (too fast to measure)\n");
        }
    }
}

/*!
 * @brief Print CSV header row with column names
 */
static void print_csv_header(void)
{
    if (csv_header_printed) {
        return;
    }

    fprintf(stdout,
            "test_name,iteration,file_size_mb,microseconds,throughput_mbs\n");
    csv_header_printed = 1;
}

/*!
 * @brief Print single CSV row with test results
 */
static void print_csv_row(const char *test_name, int iteration, off_t file_size)
{
    if (!in_warmup_phase) {
        unsigned long long d = delta();
        double throughput_mbs = (d > 0) ?
                                ((double)file_size * 1000000.0) / (MEGABYTE * (double)d) : 0.0;
        fprintf(stdout, "%s,%d,%.3f,%llu,%.2f\n",
                test_name, iteration,
                (double)file_size / MEGABYTE,
                d, throughput_mbs);
    }
}

/*!
 * @brief Print CSV statistics row with mean, median, stddev, percentiles
 */
static void print_csv_statistics(const char *test_name, off_t file_size)
{
    if (current_test_stats.count < 2) {
        return;
    }

    calculate_statistics(&current_test_stats);
    double file_size_mb = (double)file_size / MEGABYTE;
    /* Guard against division by zero */
    double mean_throughput = (current_test_stats.mean > 0) ?
                             ((double)file_size * 1000000.0) / (MEGABYTE * current_test_stats.mean) : 0.0;
    double median_throughput = (current_test_stats.median > 0) ?
                               ((double)file_size * 1000000.0) / (MEGABYTE * current_test_stats.median) : 0.0;
    double min_throughput = (current_test_stats.min > 0) ?
                            ((double)file_size * 1000000.0) / (MEGABYTE * (double)current_test_stats.min) :
                            0.0;
    double max_throughput = (current_test_stats.max > 0) ?
                            ((double)file_size * 1000000.0) / (MEGABYTE * (double)current_test_stats.max) :
                            0.0;
    fprintf(stdout, "%s_mean,,%.3f,%llu,%.2f\n",
            test_name, file_size_mb,
            (unsigned long long)current_test_stats.mean,
            mean_throughput);
    fprintf(stdout, "%s_median,,%.3f,%llu,%.2f\n",
            test_name, file_size_mb,
            (unsigned long long)current_test_stats.median,
            median_throughput);
    fprintf(stdout, "%s_stddev,,%.3f,%llu,\n",
            test_name, file_size_mb,
            (unsigned long long)current_test_stats.stddev);
    fprintf(stdout, "%s_min,,%.3f,%llu,%.2f\n",
            test_name, file_size_mb,
            current_test_stats.min,
            min_throughput);
    fprintf(stdout, "%s_max,,%.3f,%llu,%.2f\n",
            test_name, file_size_mb,
            current_test_stats.max,
            max_throughput);
}

/* Forward declaration for size sweep wrapper */
static void run_one(char *name);

/* ------------------- */

static void print_test_summary(const char *test_name,
                               const SizeSweepResult *results, int result_count)
{
    if (result_count == 0) {
        return;
    }

    if (output_format == OUTPUT_CSV) {
        /* CSV header for size sweep results */
        fprintf(stdout, "\n# File Size Performance Summary for %s\n", test_name);
        fprintf(stdout,
                "file_size_mb,mean_throughput_mbs,median_throughput_mbs,min_throughput_mbs,max_throughput_mbs,stddev_ms,mean_time_ms\n");

        for (int i = 0; i < result_count; i++) {
            off_t file_size_bytes = (off_t)(results[i].file_size_mb * MEGABYTE);
            /* Guard against division by zero */
            double max_throughput_mbs = (results[i].min_time_us > 0) ?
                                        ((double)file_size_bytes * 1000000.0) / (MEGABYTE * (double)
                                            results[i].min_time_us) : 0.0;
            double min_throughput_mbs = (results[i].max_time_us > 0) ?
                                        ((double)file_size_bytes * 1000000.0) / (MEGABYTE * (double)
                                            results[i].max_time_us) : 0.0;
            fprintf(stdout, "%.3f,%.2f,%.2f,%.2f,%.2f,%.1f,%.1f\n",
                    results[i].file_size_mb,
                    results[i].mean_throughput_mbs,
                    results[i].median_throughput_mbs,
                    min_throughput_mbs,
                    max_throughput_mbs,
                    results[i].stddev_us / 1000.0,
                    (double)results[i].mean_time_us / 1000.0);
        }

        return;
    }

    /* TEXT format */
    fprintf(stdout, "\n");
    fprintf(stdout,
            "╔══════════════════════════════════════════════════════════════════════════════════════╗\n");
    fprintf(stdout,
            "║                      File Size Performance Summary for %-30s║\n",
            test_name);
    fprintf(stdout,
            "╠══════════════════════════════════════════════════════════════════════════════════════╣\n");
    fprintf(stdout,
            "║ %8s │ %10s │ %12s │ %9s │ %9s │ %10s │ %8s ║\n",
            "Size", "Mean(MB/s)", "Median(MB/s)", "Min(MB/s)", "Max(MB/s)", "StdDev(ms)",
            "Mean(ms)");
    fprintf(stdout,
            "╠══════════════════════════════════════════════════════════════════════════════════════╣\n");

    for (int i = 0; i < result_count; i++) {
        off_t file_size_bytes = (off_t)(results[i].file_size_mb * MEGABYTE);
        /* min_time_us = fastest = max throughput, max_time_us = slowest = min throughput */
        /* Guard against division by zero */
        double max_throughput_mbs = (results[i].min_time_us > 0) ?
                                    ((double)file_size_bytes * 1000000.0) / (MEGABYTE * (double)
                                        results[i].min_time_us) : 0.0;
        double min_throughput_mbs = (results[i].max_time_us > 0) ?
                                    ((double)file_size_bytes * 1000000.0) / (MEGABYTE * (double)
                                        results[i].max_time_us) : 0.0;
        /* Format size with units: KB for <1MB, MB for >=1MB */
        char size_str[12];

        if (file_size_bytes < MEGABYTE) {
            snprintf(size_str, sizeof(size_str), "%ld KB", file_size_bytes / KILOBYTE);
        } else {
            snprintf(size_str, sizeof(size_str), "%ld MB", file_size_bytes / MEGABYTE);
        }

        fprintf(stdout,
                "║ %8s │ %10.2f │ %12.2f │ %9.2f │ %9.2f │ %10.1f │ %8.1f ║\n",
                size_str,
                results[i].mean_throughput_mbs,
                results[i].median_throughput_mbs,
                min_throughput_mbs,
                max_throughput_mbs,
                results[i].stddev_us / 1000.0,
                (double)results[i].mean_time_us / 1000.0);
    }

    fprintf(stdout,
            "╚══════════════════════════════════════════════════════════════════════════════════════╝\n");
}

/*!
 * @brief Print file size performance summary tables for all test types
 */
static void print_size_sweep_tables(void)
{
    /* Print separate table for each test type that has results */
    if (all_results.read_count > 0) {
        print_test_summary("Read", all_results.read_results, all_results.read_count);
    }

    if (all_results.write_count > 0) {
        print_test_summary("Write", all_results.write_results, all_results.write_count);
    }

    if (all_results.copy_count > 0) {
        print_test_summary("Copy", all_results.copy_results, all_results.copy_count);
    }

    if (all_results.servercopy_count > 0) {
        print_test_summary("ServerCopy", all_results.servercopy_results,
                           all_results.servercopy_count);
    }

    /* Display TCP metrics evolution summary for size sweep mode */
    if (!Local && enable_statistics) {
        tcp_analytics_session_end(&tcp_session, Conn);
    }
}

/*!
 * @brief Execute test with file size sweep mode enabled
 */
static void run_one_with_size_sweep(char *test_name)
{
    if (!size_sweep_enabled) {
        /* Normal single-size run */
        run_one(test_name);
        return;
    }

    /* Size sweep mode - initialize volume IDs to ensure proper open/close logic */
    VolID = 0xffff;
    VolID2 = 0xffff;
    off_t original_size = Size;
    /* Initialize all_results structure */
    memset(&all_results, 0, sizeof(AllTestResults));

    for (int i = 0; i < size_sweep_count; i++) {
        Size = size_sweep_values[i];

        if (output_format == OUTPUT_TEXT) {
            fprintf(stdout, "\n========================================\n");

            if (Size < MEGABYTE) {
                fprintf(stdout, "**** Testing with File Size: %ld KB ****\n", Size / KILOBYTE);
            } else {
                fprintf(stdout, "**** Testing with File Size: %ld MB ****\n", Size / MEGABYTE);
            }

            fprintf(stdout, "========================================\n");
        }

        /* Run test for this size */
        run_one(test_name);

        /* In Local mode with size sweep: reset Dir_heap to avoid hitting MAXDIR limit
         * Keep volume entry intact (Vol_heap[0]) but free all directory entries */
        if (Local && i < size_sweep_count - 1) {  /* Not after last iteration */
            for (int vol_idx = 0; vol_idx < MAXVOL; vol_idx++) {
                if (Vol_heap[vol_idx] != NULL) {
                    /* Keep Vol_heap[vol_idx] but reset directory entries */
                    for (int dir_idx = 3; dir_idx < MAXDIR; dir_idx++) {
                        if (Dir_heap[vol_idx][dir_idx] != NULL) {
                            free(Dir_heap[vol_idx][dir_idx]);
                            Dir_heap[vol_idx][dir_idx] = NULL;
                        }
                    }
                }
            }
        }

        /* Results are now collected inside each test function */
    }

    Size = original_size;  /* Restore */

    /* Close volumes after all size sweep iterations complete */
    if (VolID != 0xffff) {
        VFS.closevol(Conn, VolID);
    }

    if (*Vol2 && VolID2 != 0xffff) {
        VFS.closevol(Conn, VolID2);
    }

    /* Clean up test directories after ALL size sweep iterations complete */
    if (Local) {
        /* Local mode: use system rm -rf (POSIX operations, not AFP) */
        char cleanup_cmd[MAXPATHLEN * 2];
        int cleanup_pid = getpid();
        snprintf(cleanup_cmd, sizeof(cleanup_cmd),
                 "rm -rf %s/ReadTest-%d %s/WriteTest-%d %s/CopyTest-%d %s/ServerCopyTest-%d 2>/dev/null",
                 Vol, cleanup_pid, Vol, cleanup_pid, Vol, cleanup_pid, Vol, cleanup_pid);
        system(cleanup_cmd);
    }

    /* Display comparison tables - one per test type */
    print_size_sweep_tables();
}

/*!
 * @brief Print results and collect statistics for size sweep mode
 */
static void print_and_collect_results(const char *test_name)
{
    if (!enable_statistics || current_test_stats.count == 0) {
        return;
    }

    /* Print statistics */
    if (output_format == OUTPUT_CSV) {
        print_csv_statistics(test_name, Size);
    } else {
        print_statistics_text(test_name);
    }

    /* Collect results for size sweep mode */
    if (!size_sweep_enabled) {
        return;
    }

    SizeSweepResult result;
    result.file_size_mb = (double)Size / (double)MEGABYTE;
    result.mean_time_us = (unsigned long long)current_test_stats.mean;
    result.median_time_us = (unsigned long long)current_test_stats.median;
    result.min_time_us = current_test_stats.min;
    result.max_time_us = current_test_stats.max;
    result.stddev_us = current_test_stats.stddev;
    result.mean_throughput_mbs = (current_test_stats.mean > 0) ?
                                 ((double)Size * 1000000.0) / ((double)MEGABYTE * current_test_stats.mean) : 0.0;
    result.median_throughput_mbs = (current_test_stats.median > 0) ?
                                   ((double)Size * 1000000.0) / ((double)MEGABYTE * current_test_stats.median) :
                                   0.0;

    /* Determine which array to store in based on test_name */
    if (strcmp(test_name, "Read") == 0) {
        all_results.read_results[all_results.read_count++] = result;
    } else if (strcmp(test_name, "Write") == 0) {
        all_results.write_results[all_results.write_count++] = result;
    } else if (strcmp(test_name, "Copy") == 0) {
        all_results.copy_results[all_results.copy_count++] = result;
    } else if (strcmp(test_name, "ServerCopy") == 0) {
        all_results.servercopy_results[all_results.servercopy_count++] = result;
    }
}

/* ------------------- */
static int check_test_dir_exists(uint16_t vol, char *dir_name,
                                 const char *test_name)
{
    uint16_t dir_bitmap = (uint16_t)((1U << DIRPBIT_LNAME) | (1U << DIRPBIT_PDID));

    if (ntohl(AFPERR_NOOBJ) != VFS.getfiledirparams(Conn, vol, DIRDID_ROOT,
            dir_name,
            dir_bitmap, dir_bitmap)) {
        fprintf(stderr, "Warning: %s test skipped - directory '%s' already exists\n",
                test_name, dir_name);
        test_nottested();
        return 1;  /* exists */
    }

    return 0;  /* doesn't exist */
}

/*!
 * @brief Print iteration number marker for progress tracking
 */
static void print_iteration_marker(int iteration, int display_iter)
{
    if (output_format != OUTPUT_TEXT) {
        return;
    }

    if (in_warmup_phase) {
        fprintf(stdout, "[W%d]\t", iteration);
    } else {
        fprintf(stdout, "%d\t", display_iter);
    }
}

/*!
 * @brief Print test header with name and configuration
 */
static void print_test_header(const char *test_name)
{
    if (output_format != OUTPUT_TEXT) {
        return;
    }

    fprintf(stdout, "\n===== Test Passes for %s =====\n", test_name);
    const char *sparse_note = Sparse ? "sparse file" : "";

    if (Size < MEGABYTE) {
        fprintf(stdout, "%s quantum %ld KB, size %ld KB %s\n",
                test_name, Quantum / KILOBYTE, Size / KILOBYTE, sparse_note);
    } else {
        fprintf(stdout, "%s quantum %ld KB, size %ld MB %s\n",
                test_name, Quantum / KILOBYTE, Size / MEGABYTE, sparse_note);
    }
}

/*!
 * @brief Print test header with timestamp
 */
static void header(void)
{
    if (output_format == OUTPUT_CSV) {
        print_csv_header();
    } else {
        if (WarmupRuns > 0) {
            fprintf(stdout, "Warmup: %d runs, Measured: %d runs\n", WarmupRuns, Count);
        }

        fprintf(stdout, "run\t %9s\t%6s\n", "microsec", "MB/s");
    }
}

/*!
 * @brief Print timing results and record statistics after test iteration
 */
static void timer_footer(const char *test_name, int iteration)
{
    unsigned long long d;
    gettimeofday(&Timer_end, NULL);
    d = delta();
    /* Record timing for statistics */
    record_timing(d);

    if (output_format == OUTPUT_CSV) {
        if (!in_warmup_phase) {
            double throughput_mbs = (d > 0) ?
                                    ((double)Size * 1000000.0) / (MEGABYTE * (double)d) : 0.0;
            fprintf(stdout, "%s,%d,%.3f,%llu,%.2f\n",
                    test_name, iteration,
                    (double)Size / MEGABYTE,
                    d, throughput_mbs);
        }
    } else {
        /* Convert to MB/s: Size in bytes, d in microseconds */
        if (d > 0) {
            double throughput_mbs = ((double)Size * 1000000.0) / (MEGABYTE * (double)d);
            fprintf(stdout, "%9lld\t%.2f\n", d, throughput_mbs);
        } else {
            fprintf(stdout, "       <1\t(too fast)\n");
        }
    }
}

/*!
 * @brief Execute Write test with configured file size and iterations
 */
void Write(void)
{
    int dir = 0;
    uint16_t fork = 0;
    int id = getpid();
    static char temp[MAXPATHLEN];
    uint16_t vol = VolID;
    off_t  offset;
    off_t  offset_r;
    off_t  written;
    off_t  written_r;
    size_t nbe;
    size_t nbe_r;
    int push;
    DSI *dsi;
    reset_statistics();
    print_test_header("Write");
    header();
    sprintf(temp, "WriteTest-%d", id);

    if (check_test_dir_exists(vol, temp, "Write")) {
        return;
    }

    if (!(dir = VFS.createdir(Conn, vol, DIRDID_ROOT, temp))) {
        fprintf(stderr, "Error: Write test failed to create directory '%s'\n", temp);
        test_nottested();
        goto fin;
    }

    if (VFS.createfile(Conn, vol, 0, dir, "File")) {
        fprintf(stderr, "Error: Write test failed to create file 'File'\n");
        test_failed();
        goto fin;
    }

    dsi = &Conn->dsi;
    int total_runs = WarmupRuns + Count;

    for (int i = 1; i <= total_runs; i++) {
        /* Set warmup phase flag */
        in_warmup_phase = (i <= WarmupRuns);
        int display_iter = in_warmup_phase ? i : (i - WarmupRuns);
        fork = VFS.openfork(Conn, vol, OPENFORK_DATA, (uint16_t)(1U << FILPBIT_FNUM),
                            dir, "File",
                            OPENACC_WR | OPENACC_RD | OPENACC_DWR | OPENACC_DRD);

        if (!fork) {
            fprintf(stderr, "Error: Write test failed to open fork at iteration %d\n", i);
            test_failed();
            goto fin1;
        }

        print_iteration_marker(i, display_iter);
        gettimeofday(&Timer_start, NULL);
        nbe = nbe_r = Quantum;
        written = written_r = Size;
        offset = offset_r = 0;
        push = 0;

        while (written) {
            if (written < Quantum) {
                nbe = written;
            }

            if (push < Request && VFS.writeheader(dsi, fork, offset, nbe, Buffer, 0)) {
                test_failed();
                goto fin1;
            }

            written -= nbe;
            offset += nbe;
            push++;

            if (push >= Request) {
                if (written_r < Quantum) {
                    nbe_r = written_r;
                }

                if (VFS.writefooter(dsi, fork, offset_r, nbe_r, Buffer, 0)) {
                    test_failed();
                    goto fin1;
                }

                push--;
                written_r -= nbe_r;
                offset_r += nbe_r;
            }
        }

        while (push) {
            if (written_r < Quantum) {
                nbe_r = written_r;
            }

            if (VFS.writefooter(dsi, fork, offset_r, nbe_r, Buffer, 0)) {
                test_failed();
                goto fin1;
            }

            push--;
            written_r -= nbe_r;
            offset_r += nbe_r;
        }

        if (Flush && VFS.flushfork(Conn, fork)) {
            test_failed();
            goto fin1;
        }

        timer_footer("Write", display_iter);

        if (VFS.closefork(Conn, fork)) {
            test_failed();
            goto fin;
        }

        fork = 0;

        /* Add delay between iterations (not after last iteration) */
        if (DelaySeconds > 0 && i < total_runs) {
            sleep(DelaySeconds);
        }

        if (Delete) {
            if (VFS.delete(Conn, vol, dir, "File")
                    || VFS.createfile(Conn, vol, 0, dir, "File")) {
                test_failed();
                goto fin;
            }
        }
    }

fin1:

    if (fork && VFS.closefork(Conn, fork)) {
        fprintf(stderr, "Warning: Write cleanup - failed to close fork\n");
    }

    if (VFS.delete(Conn, vol, dir, "File")) {
        fprintf(stderr, "Warning: Write cleanup - failed to delete file 'File'\n");
    }

fin:

    if (VFS.delete(Conn, vol, dir, "")) {
        fprintf(stderr, "Warning: Write cleanup - failed to delete directory\n");
    }

    /* Print statistics and collect results */
    print_and_collect_results("Write");
    return;
}

/*!
 * @brief Initialize fork by truncating to 0 then setting to File_size
 */
int init_fork(uint16_t fork)
{
    off_t  written;
    off_t  offset;
    size_t nbe;

    if (Sparse) {
        /* assume server will create a sparse file */
        if (VFS.setforkparam(Conn, fork, (uint16_t)(1U << FILPBIT_DFLEN), Size)) {
            return -1;
        }

        return 0;
    }

    nbe = Quantum;
    written = Size;
    offset = 0;

    while (written) {
        if (written < Quantum) {
            nbe = written;
        }

        if (VFS.write(Conn, fork, offset, nbe, Buffer, 0)) {
            return -1;
        }

        written -= nbe;
        offset += nbe;
    }

    if (Flush && VFS.flushfork(Conn, fork)) {
        return -1;
    }

    return 0;
}

/*!
 * @brief Get file descriptor from fork (AFP or Local mode)
 */
static int getfd(CONN *conn, int fork)
{
    DSI *dsi;

    if (Local) {
        return fork;
    }

    dsi = &Conn->dsi;
    return dsi->socket;
}

/*!
 * @brief Execute Copy test with configured file size and iterations
 */
void Copy(void)
{
    int dir = 0;
    int dir2 = 0;
    uint16_t fork = 0;
    uint16_t fork2 = 0;
    int fork_fd;
    int fork2_fd;
    int id = getpid();
    static char temp[MAXPATHLEN];
    uint16_t vol = VolID;
    uint16_t vol2 = VolID2;
    off_t  written;
    off_t  written_r;
    off_t  written_w;
    off_t  offset = 0;
    off_t  offset_r = 0;
    off_t  offset_w = 0;
    size_t nbe;
    size_t nbe_r;
    size_t nbe_w;
    int push;
    DSI *dsi;
    int max = Request;
    int cnt = 0;
    dsi = &Conn->dsi;
    reset_statistics();
    print_test_header("Copy");
    header();
    sprintf(temp, "CopyTest-%d", id);

    if (!Filename) {
        if (check_test_dir_exists(vol, temp, "Copy")) {
            return;
        }
    } else {
        dir = DIRDID_ROOT;

        if (ntohl(AFP_OK) != is_there(Conn, VolID, dir, Filename)) {
            test_nottested();
            return;
        }
    }

    if (VolID != VolID2
            && ntohl(AFPERR_NOOBJ) != is_there(Conn, VolID2, DIRDID_ROOT, temp)) {
        test_nottested();
        return;
    }

    if (!Filename && !(dir = VFS.createdir(Conn, vol, DIRDID_ROOT, temp))) {
        test_nottested();
        goto fin;
    }

    if (VolID == VolID2) {
        dir2 = dir;
    } else if (!(dir2 = VFS.createdir(Conn, vol2, DIRDID_ROOT, temp))) {
        test_nottested();
        goto fin;
    }

    if (!Filename) {
        strcpy(temp, "Source");

        if (VFS.createfile(Conn, vol, 0, dir, temp)) {
            test_failed();
            goto fin;
        }
    } else {
        strcpy(temp, Filename);
    }

    if (VFS.createfile(Conn, vol2,  0, dir2, "Destination")) {
        test_failed();
        goto fin;
    }

    int total_runs = WarmupRuns + Count;

    for (int i = 1; i <= total_runs; i++) {
        /* Set warmup phase flag */
        in_warmup_phase = (i <= WarmupRuns);
        int display_iter = in_warmup_phase ? i : (i - WarmupRuns);
        fork = VFS.openfork(Conn, vol, OPENFORK_DATA, (uint16_t)(1U << FILPBIT_FNUM),
                            dir, temp,
                            OPENACC_WR | OPENACC_RD | OPENACC_DWR | OPENACC_DRD);

        if (!fork) {
            test_failed();
            goto fin1;
        }

        fork2 = VFS.openfork(Conn, vol2, OPENFORK_DATA, (uint16_t)(1U << FILPBIT_FNUM),
                             dir2,
                             "Destination", OPENACC_WR | OPENACC_RD | OPENACC_DWR | OPENACC_DRD);

        if (!fork2) {
            test_failed();
            goto fin1;
        }

        if (!Filename && (i == 1 || Delete)) {
            if (init_fork(fork)) {
                test_failed();
                goto fin1;
            }
        }

        fork_fd = getfd(Conn, fork);
        fork2_fd = getfd(Conn, fork2);
        print_iteration_marker(i, display_iter);
        gettimeofday(&Timer_start, NULL);
        nbe = nbe_r = nbe_w = Quantum;
        written = written_r = written_w = Size;
        offset = offset_r = offset_w = 0;
        push = 0;
        cnt = 0;

        while (written) {
            if (written < Quantum) {
                nbe = written;
            }

            cnt++;

            if (push < max && VFS.readheader(dsi, fork, offset, nbe, Buffer)) {
                test_failed();
                goto fin1;
            }

            written -= nbe;
            offset += nbe;
            push++;

            if (push >= max) {
                if (written_r < Quantum) {
                    nbe_r = written_r;
                }

                if (VFS.readfooter(dsi, fork, offset_r, nbe_r, Buffer)) {
                    test_failed();
                    goto fin1;
                }

                if (VFS.writeheader(dsi, fork2, offset_r, nbe_r, Buffer, 0)) {
                    test_failed();
                    goto fin1;
                }

                if (cnt >= max * 2 - 1) {
                    /* Adjust write size if remaining data is smaller than quantum */
                    if (written_w < Quantum) {
                        nbe_w = written_w;
                    }

                    if (VFS.writefooter(dsi, fork2, offset_w, nbe_w, Buffer, 0)) {
                        test_failed();
                        goto fin1;
                    }

                    written_w -= nbe_w;
                    offset_w += nbe_w;
                }

                push--;
                written_r -= nbe_r;
                offset_r += nbe_r;
            }
        }

        while (push) {
            if (written_r < Quantum) {
                nbe_r = written_r;
            }

            if (VFS.readfooter(dsi, fork, offset_r, nbe_r, Buffer)) {
                test_failed();
                goto fin1;
            }

            if (VFS.writeheader(dsi, fork2, offset_r, nbe_r, Buffer, 0)) {
                test_failed();
                goto fin1;
            }

            if (VFS.writefooter(dsi, fork2, offset_w, nbe_w, Buffer, 0)) {
                test_failed();
                goto fin1;
            }

            push--;
            written_r -= nbe_r;
            offset_r += nbe_r;
            written_w -= nbe_w;
            offset_w += nbe_w;
        }

        for (push = 1; push < max; push++) {
            if (written_w < Quantum) {
                nbe_w = written_w;
            }

            if (VFS.writefooter(dsi, fork2, offset_w, nbe_w, Buffer, 0)) {
                test_failed();
                goto fin1;
            }

            written_w -= nbe_w;
            offset_w += nbe_w;
        }

        timer_footer("Copy", display_iter);

        if (VFS.closefork(Conn, fork)) {
            test_failed();
            goto fin1;
        }

        fork = 0;

        if (VFS.closefork(Conn, fork2)) {
            test_failed();
            goto fin1;
        }

        fork2 = 0;

        /* Add delay between iterations (not after last iteration) */
        if (DelaySeconds > 0 && i < total_runs) {
            sleep(DelaySeconds);
        }

        if (Delete) {
            if (!Filename && VFS.delete(Conn, vol, dir, temp)) {
                goto fin;
            }

            if (VFS.delete(Conn, vol2,  dir2, "Destination")) {
                goto fin;
            }

            if (!Filename && VFS.createfile(Conn, vol, 0, dir, temp)) {
                test_failed();
                goto fin;
            }

            if (VFS.createfile(Conn, vol2,  0, dir2, "Destination")) {
                test_failed();
                goto fin;
            }
        }
    }

fin1:

    if (fork && VFS.closefork(Conn, fork)) {
        fprintf(stderr, "Warning: Copy cleanup - failed to close fork\n");
    }

    if (fork2 && VFS.closefork(Conn, fork2)) {
        fprintf(stderr, "Warning: Copy cleanup - failed to close fork2\n");
    }

    if (!Filename && VFS.delete(Conn, vol, dir, temp)) {
        fprintf(stderr, "Warning: Copy cleanup - failed to delete source file\n");
    }

    if (VFS.delete(Conn, vol2,  dir2, "Destination")) {
        fprintf(stderr, "Warning: Copy cleanup - failed to delete destination file\n");
    }

fin:

    if (!Filename && dir && VFS.delete(Conn, vol, dir, "")) {
        fprintf(stderr, "Warning: Copy cleanup - failed to delete source directory\n");
    }

    if (dir2 && dir2 != dir && VFS.delete(Conn, vol2,  dir2, "")) {
        fprintf(stderr,
                "Warning: Copy cleanup - failed to delete destination directory\n");
    }

    /* Print statistics and collect results */
    print_and_collect_results("Copy");
    return;
}

/*!
 * @brief Execute ServerCopy test with configured file size and iterations
 */
void ServerCopy(void)
{
    int dir = 0;
    int dir2 = 0;
    uint16_t fork = 0;
    int id = getpid();
    static char temp[MAXPATHLEN];
    uint16_t vol = VolID;
    uint16_t vol2 = VolID2;

    /* ServerCopy requires AFP server - skip in Local mode */
    if (Local) {
        if (output_format == OUTPUT_TEXT) {
            fprintf(stdout, "\n===== Test Passes for ServerCopy =====\n");
            fprintf(stdout,
                    "ServerCopy test skipped in Local mode (requires AFP server)\n");
        }

        test_skipped(0);  /* Skip reason: Local mode - ServerCopy requires AFP server */
        return;
    }

    reset_statistics();
    print_test_header("ServerCopy");
    header();
    sprintf(temp, "ServerCopyTest-%d", id);

    if (check_test_dir_exists(vol, temp, "ServerCopy")) {
        return;
    }

    if (VolID != VolID2
            && ntohl(AFPERR_NOOBJ) != is_there(Conn, VolID2, DIRDID_ROOT, temp)) {
        test_nottested();
        return;
    }

    if (!(dir = VFS.createdir(Conn, vol, DIRDID_ROOT, temp))) {
        test_nottested();
        goto fin;
    }

    if (VolID == VolID2) {
        dir2 = dir;
    } else if (!(dir2 = VFS.createdir(Conn, vol2, DIRDID_ROOT, temp))) {
        test_nottested();
        goto fin;
    }

    if (VFS.createfile(Conn, vol, 0, dir, "Source")) {
        test_failed();
        goto fin;
    }

    int total_runs = WarmupRuns + Count;

    for (int i = 1; i <= total_runs; i++) {
        /* Set warmup phase flag */
        in_warmup_phase = (i <= WarmupRuns);
        int display_iter = in_warmup_phase ? i : (i - WarmupRuns);
        fork = VFS.openfork(Conn, vol, OPENFORK_DATA, (uint16_t)(1U << FILPBIT_FNUM),
                            dir,
                            "Source", OPENACC_WR | OPENACC_RD);

        if (!fork) {
            test_failed();
            goto fin1;
        }

        if (i == 1 || Delete) {
            if (init_fork(fork)) {
                test_failed();
                goto fin1;
            }
        }

        if (VFS.closefork(Conn, fork)) {
            test_failed();
            goto fin1;
        }

        fork = 0;
        print_iteration_marker(i, display_iter);
        gettimeofday(&Timer_start, NULL);

        if (VFS.copyfile(Conn, vol, dir, vol2, dir2, "Source", "", "Destination")) {
            test_failed();
            goto fin1;
        }

        timer_footer("ServerCopy", display_iter);

        if (Delete) {
            if (VFS.delete(Conn, vol, dir, "Source")) {
                test_failed();
                goto fin;
            }

            if (VFS.createfile(Conn, vol, 0, dir, "Source")) {
                test_failed();
                goto fin;
            }
        }

        if (VFS.delete(Conn, vol2,  dir2, "Destination")) {
            test_failed();
            goto fin;
        }
    }

fin1:

    if (fork && VFS.closefork(Conn, fork)) {
        fprintf(stderr, "Warning: ServerCopy cleanup - failed to close fork\n");
    }

    if (VFS.delete(Conn, vol, dir, "Source")) {
        fprintf(stderr, "Warning: ServerCopy cleanup - failed to delete source file\n");
    }

    VFS.delete(Conn, vol2,  dir2, "Destination");
fin:

    if (dir && VFS.delete(Conn, vol, dir, "")) {
        fprintf(stderr,
                "Warning: ServerCopy cleanup - failed to delete source directory\n");
    }

    if (dir2 && dir2 != dir && VFS.delete(Conn, vol2,  dir2, "")) {
        fprintf(stderr,
                "Warning: ServerCopy cleanup - failed to delete destination directory\n");
    }

    /* Print statistics and collect results */
    print_and_collect_results("ServerCopy");
    return;
}

/*!
 * @brief Execute Read test with configured file size and iterations
 */
void Read(void)
{
    int dir = 0;
    uint16_t fork = 0;
    int id = getpid();
    static char temp[MAXPATHLEN];
    uint16_t vol = VolID;
    off_t  written;
    off_t  written_r;
    off_t  offset = 0;
    off_t  offset_r = 0;
    size_t nbe;
    size_t nbe_r;
    DSI *dsi;
    int push;
    reset_statistics();
    print_test_header("Read");
    header();

    if (!Filename) {
        sprintf(temp, "ReadTest-%d", id);

        if (check_test_dir_exists(vol, temp, "Read")) {
            return;
        }

        if (!(dir = VFS.createdir(Conn, vol, DIRDID_ROOT, temp))) {
            fprintf(stderr, "Error: Read test failed to create directory '%s'\n", temp);
            test_nottested();
            goto fin;
        }

        if (VFS.createfile(Conn, vol, 0, dir, "File")) {
            fprintf(stderr, "Error: Read test failed to create file 'File'\n");
            test_failed();
            goto fin;
        }

        strcpy(temp, "File");
    } else {
        dir = DIRDID_ROOT;
        strcpy(temp, Filename);

        if (ntohl(AFP_OK) != is_there(Conn, VolID, dir, temp)) {
            test_nottested();
            return;
        }
    }

    dsi = &Conn->dsi;
    int total_runs = WarmupRuns + Count;

    for (int i = 1; i <= total_runs; i++) {
        /* Set warmup phase flag */
        in_warmup_phase = (i <= WarmupRuns);
        int display_iter = in_warmup_phase ? i : (i - WarmupRuns);
        fork = VFS.openfork(Conn, vol, OPENFORK_DATA, (uint16_t)(1U << FILPBIT_FNUM),
                            dir, temp,
                            OPENACC_WR | OPENACC_RD | OPENACC_DWR | OPENACC_DRD);

        if (!fork) {
            test_failed();
            goto fin1;
        }

        if (!Filename && (i == 1 || Delete)) {
            if (init_fork(fork)) {
                test_failed();
                goto fin1;
            }
        }

        nbe = nbe_r = Quantum;
        written = Size;
        written_r = Size;
        offset = 0;
        offset_r = 0;
        push = 0;
        print_iteration_marker(i, display_iter);
        gettimeofday(&Timer_start, NULL);

        while (written) {
            if (written < Quantum) {
                nbe = written;
            }

            if (push < Request && VFS.readheader(dsi, fork, offset, nbe, Buffer)) {
                test_failed();
                goto fin1;
            }

            written -= nbe;
            offset += nbe;
            push++;

            if (push >= Request) {
                if (written_r < Quantum) {
                    nbe_r = written_r;
                }

                if (VFS.readfooter(dsi, fork, offset_r, nbe_r, Buffer)) {
                    test_failed();
                    goto fin1;
                }

                push--;
                written_r -= nbe_r;
                offset_r += nbe_r;
            }
        }

        while (push) {
            if (written_r < Quantum) {
                nbe_r = written_r;
            }

            if (VFS.readfooter(dsi, fork, offset_r, nbe_r, Buffer)) {
                test_failed();
                goto fin1;
            }

            push--;
            written_r -= nbe_r;
            offset_r += nbe_r;
        }

        timer_footer("Read", display_iter);

        if (VFS.closefork(Conn, fork)) {
            test_failed();
        }

        fork = 0;

        /* Add delay between iterations (not after last iteration) */
        if (DelaySeconds > 0 && i < total_runs) {
            sleep(DelaySeconds);
        }

        if (!Filename && Delete) {
            if (VFS.delete(Conn, vol, dir, temp)) {
                goto fin;
            }

            if (VFS.createfile(Conn, vol, 0, dir, temp)) {
                test_failed();
                goto fin;
            }
        }
    }

fin1:

    if (fork && VFS.closefork(Conn, fork)) {
        fprintf(stderr, "Warning: Read cleanup - failed to close fork\n");
    }

    if (!Filename && VFS.delete(Conn, vol, dir, temp)) {
        fprintf(stderr, "Warning: Read cleanup - failed to delete file\n");
    }

fin:

    if (!Filename && VFS.delete(Conn, vol, dir, "")) {
        fprintf(stderr, "Warning: Read cleanup - failed to delete directory\n");
    }

    /* Print statistics and collect results */
    print_and_collect_results("Read");
    return;
}



/* ----------- */

struct test_entry {
    const char *name;
    void (*fn)(void);
};

static struct test_entry test_table[] = {
    { "Read", Read },
    { "Write", Write },
    { "Copy", Copy },
    { "ServerCopy", ServerCopy },
    { NULL, NULL }
};

/*!
 * @brief Execute single test by name (Read/Write/Copy/ServerCopy)
 */
static void run_one(char *name)
{
    char *token;
    char *tp;

    /* Print welcome message and test configuration */
    if (output_format == OUTPUT_TEXT) {
        fprintf(stdout, "\n");
        fprintf(stdout, "AFP Speedtest - Configuration\n");
        fprintf(stdout,
                "════════════════════════════════════════\n");

        if (Local) {
            fprintf(stdout, " Mode:            Local (Direct Filesystem I/O)\n");
            fprintf(stdout, " AFP Version:     N/A (Local)\n");
            fprintf(stdout, " Directory:       %s\n", Vol);
        } else {
            fprintf(stdout, " Server:          %s\n", Server);
            fprintf(stdout, " AFP Version:     %s\n", vers);
            fprintf(stdout, " Volume:          %s\n", Vol);
        }

        fprintf(stdout, " Tests:           %s\n", name);

        if (Size < MEGABYTE) {
            fprintf(stdout, " File Size:       %ld KB\n", Size / KILOBYTE);
        } else {
            fprintf(stdout, " File Size:       %ld MB\n", Size / MEGABYTE);
        }

        fprintf(stdout, " Iterations:      %d\n", Count);
        fprintf(stdout, " Warmup Runs:     %d\n", WarmupRuns);

        if (DelaySeconds > 0) {
            fprintf(stdout, " Delay:           %d seconds\n", DelaySeconds);
        }

        if (enable_statistics) {
            fprintf(stdout, " Statistics:      Enabled\n");
        }

        fprintf(stdout, "\n");
    }

    dsi = &Conn->dsi;

    /* In size sweep mode, volumes remain open across size iterations.
     * Only open if not already open (VolID == 0xffff means not yet opened). */
    if (!size_sweep_enabled || VolID == 0xffff) {
        press_enter("Opening volume.");
        VolID = VFS.openvol(Conn, Vol);

        if (VolID == 0xffff) {
            fprintf(stderr, "Error: Failed to open volume/directory '%s'\n", Vol);
            test_nottested();
            return;
        }
    }

    if (*Vol2) {
        /* Only open Vol2 if not already open */
        if (!size_sweep_enabled || VolID2 == 0xffff) {
            VolID2 = VFS.openvol(Conn, Vol2);

            if (VolID2 == 0xffff) {
                test_nottested();
                return;
            }
        }
    } else {
        VolID2 = VolID;
    }

    /* Capture network metrics before this test */
    if (!Local && enable_statistics) {
        /* For size sweep: initialize session on first test */
        if (size_sweep_enabled &&
                all_results.read_count == 0 &&
                all_results.write_count == 0 &&
                all_results.copy_count == 0 &&
                all_results.servercopy_count == 0) {
            tcp_analytics_session_start(&tcp_session, Conn);
        }

        tcp_analytics_test_start(&tcp_session, Conn, Size);
    }

    /* check server quantum size */
    if (!Quantum) {
        Quantum = dsi->server_quantum;
    } else if (Quantum > dsi->server_quantum) {
        fprintf(stdout, "\t server quantum (%d) too small\n", dsi->server_quantum);
        return;
    }

    if (Direct) {
        /* assume correctly aligned for O_DIRECT */
        Buffer = mmap(NULL, Quantum, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    } else {
        Buffer = malloc(Quantum);
    }

    if (!Buffer) {
        fprintf(stdout, "\t can't allocate (%ld) bytes\n", Quantum);
        return;
    }

    tp = strdup(name);

    if (!Quiet) {
        fprintf(stdout, "Running test(s): %s\n", tp);
    }

    token = strtok(tp, ",");

    while (token) {
        if (!Quiet) {
            fprintf(stdout, "Running test: %s\n", token);
        }

        press_enter(token);
        int found = 0;

        for (int i = 0; test_table[i].name; i++) {
            if (!strcmp(test_table[i].name, token)) {
                test_table[i].fn();
                found = 1;
                break;
            }
        }

        if (!found) {
            fprintf(stderr, "Unknown test: %s\n", token);
            test_nottested();
        }

        token = strtok(NULL, ",");
    }

    free(tp);

    /* Capture network metrics after this test and print comparison */
    if (!Local && enable_statistics) {
        tcp_analytics_test_end(&tcp_session, Conn);
    }

    /* In size sweep mode, keep volumes open across all size iterations
     * Only close after all sizes tested (handled in run_one_with_size_sweep) */
    if (!size_sweep_enabled) {
        VFS.closevol(Conn, VolID);

        if (*Vol2) {
            VFS.closevol(Conn, VolID2);
        }

        /* Clean up test directories - only in non-sweep mode or after all sweeps complete */
        if (Local) {
            /* Local mode: use system rm -rf (POSIX operations, not AFP) */
            char cleanup_cmd[MAXPATHLEN * 2];
            int cleanup_pid = getpid();
            snprintf(cleanup_cmd, sizeof(cleanup_cmd),
                     "rm -rf %s/ReadTest-%d %s/WriteTest-%d %s/CopyTest-%d %s/ServerCopyTest-%d 2>/dev/null",
                     Vol, cleanup_pid, Vol, cleanup_pid, Vol, cleanup_pid, Vol, cleanup_pid);
            system(cleanup_cmd);
        }
    }

    /* AFP mode: tests clean up properly via VFS.delete() in fin: sections */
}

/*!
 * @brief Display usage information and exit
 */
void usage(char *av0)
{
    fprintf(stdout,
            "usage:\t%s [-1234567acDeiLTVvy] [-h host] [-p port] [-s vol] [-P path] [-S vol2] [-u user] [-w password] [-n iterations] [-W warmup] "
            "[-t delay] [-d size] [-z sizes] [-q quantum] [-r requests] [-f test] [-F file] \n",
            av0);
    fprintf(stdout, "\t-h\tserver host name (default localhost)\n");
    fprintf(stdout, "\t-p\tserver port (default 548)\n");
    fprintf(stdout, "\t-s\tvolume/share to mount (AFP mode)\n");
    fprintf(stdout, "\t-P\tlocal directory path (Local mode with -L)\n");
    fprintf(stdout, "\t-S\tsecond volume to mount\n");
    fprintf(stdout, "\t-u\tuser name (default uid)\n");
    fprintf(stdout, "\t-w\tpassword\n");
    fprintf(stdout, "\t-c\toutput in CSV format (auto-enables statistics)\n");
    fprintf(stdout, "\t-L\tuse posix calls (default AFP calls)\n");
    fprintf(stdout,
            "\t-D\tdisable O_DIRECT in Local mode (default: auto-enabled)\n");
    fprintf(stdout, "\t-1\tAFP 2.1 version\n");
    fprintf(stdout, "\t-2\tAFP 2.2 version\n");
    fprintf(stdout, "\t-3\tAFP 3.0 version\n");
    fprintf(stdout, "\t-4\tAFP 3.1 version\n");
    fprintf(stdout, "\t-5\tAFP 3.2 version\n");
    fprintf(stdout, "\t-6\tAFP 3.3 version\n");
    fprintf(stdout, "\t-7\tAFP 3.4 version (default)\n");
    fprintf(stdout, "\t-n\thow many iterations to run (default: 1)\n");
    fprintf(stdout, "\t-W\twarmup runs excluded from statistics (default: 1)\n");
    fprintf(stdout, "\t-T\tshow statistics (auto-enabled when n > 1)\n");
    fprintf(stdout,
            "\t-t\tdelay in seconds between test iterations (default: 0)\n");
    fprintf(stdout, "\t-d\tfile size (Mbytes, default 64)\n");
    fprintf(stdout,
            "\t-z\tfile size sweep, comma-separated list in MB (e.g., '0.004,1,2,4,8,16') (max 1024)\n");
    fprintf(stdout, "\t-q\tpacket size (Kbytes, default server quantum)\n");
    fprintf(stdout,
            "\t-r\tnumber of outstanding requests for pipelining (default 1)\n");
    fprintf(stdout, "\t-y\tuse a new file for each run (default same file)\n");
    fprintf(stdout, "\t-e\tsparse file (default no)\n");
    fprintf(stdout, "\t-a\tdon't flush to disk after write (default yes)\n");
    fprintf(stdout,
            "\t-F\tread from file in volume root folder (default create a temporary file)\n");
    fprintf(stdout, "\t-v\tverbose (default no)\n");
    fprintf(stdout, "\t-V\tvery verbose (default no)\n");
    fprintf(stdout,
            "\t-f\ttest(s) to run, comma-separated list (Read,Write,Copy,ServerCopy - default Write)  \n");
    fprintf(stdout,
            "\t-i\tinteractive mode, prompts before every test (debug purposes)\n");
    exit(1);
}

/*!
 * @brief Main entry point: parse arguments, setup connection, run tests
 */
int main(int ac, char **av)
{
    int cc;

    if (ac == 1) {
        usage(av[0]);
    }

    while ((cc = getopt(ac, av,
                        "1234567aceDiLTVvyd:F:f:h:n:p:P:q:r:S:s:t:u:w:W:z:")) != EOF) {
        switch (cc) {
        case '1':
            vers = "AFPVersion 2.1";
            Version = 21;
            break;

        case '2':
            vers = "AFP2.2";
            Version = 22;
            break;

        case '3':
            vers = "AFPX03";
            Version = 30;
            break;

        case '4':
            vers = "AFP3.1";
            Version = 31;
            break;

        case '5':
            vers = "AFP3.2";
            Version = 32;
            break;

        case '6':
            vers = "AFP3.3";
            Version = 33;
            break;

        case '7':
            vers = "AFP3.4";
            Version = 34;
            break;

        case 'a':
            Flush = 0;
            break;

        case 'c':
            output_format = OUTPUT_CSV;
            Quiet = 1;  /* CSV mode is always quiet */
            enable_statistics = 1;  /* Auto-enable statistics for CSV */
            break;

        case 'D':
            /* -D flag: disable O_DIRECT in Local mode (auto-enabled by default) */
            Direct = 0;
            break;

        case 'd':
            Size = atoi(optarg) * MEGABYTE;
            break;

        case 'e':
            Sparse = 1;
            break;

        case 'F':
            Filename = strdup(optarg);
            break;

        case 'f' :
            Test = strdup(optarg);
            break;

        case 'h':
            Server = strdup(optarg);
            break;

        case 'i':
            Interactive = 1;
            break;

        case 'L':
            Local = 1;
            break;

        case 'n':
            Count = atoi(optarg);
            break;

        case 'p' :
            Port = atoi(optarg);

            if (Port <= 0) {
                fprintf(stdout, "Bad port.\n");
                exit(1);
            }

            break;

        case 'P':
            LocalPath = strdup(optarg);
            break;

        case 'q':
            Quantum = atoi(optarg) * KILOBYTE;
            break;

        case 'r':
            Request = atoi(optarg);
            break;

        case 'S':
            Vol2 = strdup(optarg);
            break;

        case 's':

            /* Ignore -s in Local mode (use -P instead) */
            if (!Local) {
                Vol = strdup(optarg);
            }

            break;

        case 'T':
            enable_statistics = 1;
            break;

        case 't':
            DelaySeconds = atoi(optarg);

            if (DelaySeconds < 0) {
                fprintf(stderr, "Invalid delay: %s (must be >= 0)\n", optarg);
                exit(1);
            }

            break;

        case 'u':
            User = strdup(optarg);
            break;

        case 'V':
            Quiet = 0;
            Verbose = 1;
            break;

        case 'v':
            Quiet = 0;
            break;

        case 'w':
            Password = strdup(optarg);
            break;

        case 'W':
            WarmupRuns = atoi(optarg);

            if (WarmupRuns < 0 || WarmupRuns > MAX_ITERATIONS) {
                fprintf(stderr, "Invalid warmup runs: %s (max %d)\n",
                        optarg, MAX_ITERATIONS);
                exit(1);
            }

            break;

        case 'y':
            Delete = 1;
            break;

        case 'z': {
            char *token = strtok(optarg, ",");

            while (token && size_sweep_count < MAX_SIZE_SWEEP) {
                double size_mb = atof(token);

                if (size_mb < 0.004 || size_mb > 1024) {
                    fprintf(stderr,
                            "Invalid size in sweep: %s (must be between 0.004 and 1024 MB)\n", token);
                    exit(1);
                }

                size_sweep_values[size_sweep_count++] = (off_t)(size_mb * MEGABYTE);
                token = strtok(NULL, ",");
            }

            if (size_sweep_count == 0) {
                fprintf(stderr, "No valid sizes provided for sweep\n");
                exit(1);
            }

            size_sweep_enabled = 1;
            /* Auto-enable statistics for size sweep */
            enable_statistics = 1;
        }
        break;

        default :
            usage(av[0]);
        }
    }

    /* Set default size sweep if no -d or -z was specified */
    if (!size_sweep_enabled && Size == 64 * MEGABYTE) {
        /* Default sweep: 4K,8K,16K,32K,64K,128K,256K,512K,1M,2M,4M,8M,16M,32M,64M,128M,256M,512M */
        size_sweep_values[0] = 4 * KILOBYTE;
        size_sweep_values[1] = 8 * KILOBYTE;
        size_sweep_values[2] = 16 * KILOBYTE;
        size_sweep_values[3] = 32 * KILOBYTE;
        size_sweep_values[4] = 64 * KILOBYTE;
        size_sweep_values[5] = 128 * KILOBYTE;
        size_sweep_values[6] = 256 * KILOBYTE;
        size_sweep_values[7] = 512 * KILOBYTE;
        size_sweep_values[8] = 1 * MEGABYTE;
        size_sweep_values[9] = 2 * MEGABYTE;
        size_sweep_values[10] = 4 * MEGABYTE;
        size_sweep_values[11] = 8 * MEGABYTE;
        size_sweep_values[12] = 16 * MEGABYTE;
        size_sweep_values[13] = 32 * MEGABYTE;
        size_sweep_values[14] = 64 * MEGABYTE;
        size_sweep_values[15] = 128 * MEGABYTE;
        size_sweep_values[16] = 256 * MEGABYTE;
        size_sweep_values[17] = 512 * MEGABYTE;
        size_sweep_count = 18;
        size_sweep_enabled = 1;
        enable_statistics = 1;
    }

    /* Validate Local mode requirements */
    if (Local) {
        if (LocalPath == NULL || LocalPath[0] == '\0') {
            fprintf(stderr, "Error: Local mode (-L) requires a directory path with -P\n");
            fprintf(stderr, "Example: afp_speedtest -L -P /path/to/directory\n");
            exit(1);
        }

        /* Use LocalPath as Vol for local operations */
        Vol = LocalPath;

        /* Ensure the directory exists - try mkdir, accept EEXIST */
        if (mkdir(Vol, 0750) != 0) {
            if (errno == EEXIST) {
                /* Directory exists - verify it's actually a directory */
                struct stat st;

                if (stat(Vol, &st) != 0) {
                    fprintf(stderr, "Error: Cannot stat '%s': %s\n", Vol, strerror(errno));
                    exit(1);
                }

                if (!S_ISDIR(st.st_mode)) {
                    fprintf(stderr, "Error: '%s' exists but is not a directory\n", Vol);
                    exit(1);
                }
            } else {
                fprintf(stderr, "Error: Failed to create directory '%s': %s\n", Vol,
                        strerror(errno));
                exit(1);
            }
        } else {
            fprintf(stderr, "INFO: Created directory: %s\n", Vol);
        }

        /* Always warn about ignored AFP parameters in Local mode (important info) */
        fprintf(stderr,
                "INFO: Local mode (-L) active - ignoring AFP parameters (-h, -p, -u, -w, -s)\n");
        fprintf(stderr, "INFO: Using local filesystem path: %s\n", Vol);
        fprintf(stderr, "INFO: O_DIRECT mode: %s%s\n",
                Direct ? "ENABLED" : "DISABLED",
                Direct ? " (bypasses kernel page cache for realistic AFP comparison)" :
                " (buffered I/O testing)");
    } else {
        /* Normal AFP mode validation */
        /* O_DIRECT is only relevant in Local mode - disable for AFP */
        Direct = 0;

        if (!Quiet) {
            fprintf(stdout, "Connecting to host %s:%d\n", Server, Port);
        }

        if (User != NULL && User[0] == '\0') {
            fprintf(stdout, "Error: Define a user with -u\n");
        }

        if (Password != NULL && Password[0] == '\0') {
            fprintf(stdout, "Error: Define a password with -w\n");
        }

        if (Vol != NULL && Vol[0] == '\0') {
            fprintf(stdout, "Error: Define a volume with -s\n");
        }
    }

    /* Validate total iterations */
    if (WarmupRuns + Count > MAX_ITERATIONS) {
        fprintf(stderr, "Total runs (%d warmup + %d test) exceeds max %d\n",
                WarmupRuns, Count, MAX_ITERATIONS);
        exit(1);
    }

    /* Auto-enable statistics when multiple iterations */
    if (Count > 1 && !enable_statistics) {
        enable_statistics = 1;
    }

    /* Initialize TCP analytics session */
    if (!Local && enable_statistics) {
        tcp_analytics_init(&tcp_session,
                           output_format == OUTPUT_CSV ? TCP_OUTPUT_CSV : TCP_OUTPUT_TEXT,
                           size_sweep_enabled);
    }

    /**************************
     Connection */

    if ((Conn = (CONN *)calloc(1, sizeof(CONN))) == NULL) {
        return 1;
    }

    if (Local) {
        /* Configure Local VFS */
        Local_VFS_Quiet = Quiet;
        Local_VFS_Direct = Direct;
        Dsi = &Conn->dsi;
        dsi = Dsi;
        /* Use 1 MB quantum for Local mode - no network overhead, optimized for filesystem I/O */
        dsi->server_quantum = 1 * MEGABYTE;
        VFS = local_VFS;
    } else {
        int sock;
        Dsi = &Conn->dsi;
        dsi = Dsi;
        sock = OpenClientSocket(Server, Port);

        if (sock < 0) {
            return 2;
        }

        Dsi->socket = sock;

        /* login */
        if (Version >= 30) {
            FPopenLoginExt(Conn, vers, uam, User, Password);
        } else {
            FPopenLogin(Conn, vers, uam, User, Password);
        }
    }

    Conn->afp_version = Version;
    /*********************************
    */
    run_one_with_size_sweep(Test);

    if (!Local) {
        FPLogOut(Conn);
    }

    return ExitCode;
}
