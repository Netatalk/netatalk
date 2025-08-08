/*
 * Copyright (c) 2025, Andy Lemin (andylemin)
 * Credits; Based on work by Rafal Lewczuk, Didier Gautheron, Frank Lahm, and Netatalk contributors
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

#include "specs.h"

#include <getopt.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <math.h>
#include <stdint.h>
#include <limits.h>

#ifdef __linux__
#include "lantest_io_monitor.h"
#endif

#include "afpclient.h"
#include "test.h"

/* IO Monitoring measurement types */
#define MEASURE_TIME_MS 0
#define MEASURE_AFPD_READ_IO 1
#define MEASURE_AFPD_WRITE_IO 2
#define MEASURE_CNID_READ_IO 3
#define MEASURE_CNID_WRITE_IO 4
#define NUM_MEASUREMENTS 5

#define FPWRITE_RPLY_SIZE 24
#define FPWRITE_RQST_SIZE 36

/* Test IDs */
#define TEST_OPENSTATREAD 0
#define TEST_WRITE100MB 1
#define TEST_READ100MB 2
#define TEST_LOCKUNLOCK 3
#define TEST_CREATE2000FILES 4
#define TEST_ENUM2000FILES 5
#define TEST_DELETE2000FILES 6
#define TEST_DIRTREE 7
#define TEST_CACHE_HITS 8
#define TEST_MIXED_CACHE_OPS 9
#define TEST_DEEP_TRAVERSAL 10
#define TEST_CACHE_VALIDATION 11
#define LASTTEST TEST_CACHE_VALIDATION
#define NUMTESTS (LASTTEST+1)
#define TOTAL_AFP_OPS 80686

CONN *Conn;
int ExitCode = 0;
int Version = 34;
int Mac = 0;
char Data[300000] = "";
char    *Vol = "";
char    *User = "";
char    *Path;
char    *Test = NULL;
static uint16_t vol;
static DSI *dsi;
static char    *Server = "localhost";
static int32_t Port = DSI_AFPOVERTCP_PORT;
static char    *Password = "";
static uint8_t Iterations = 2;
static uint8_t Iterations_save;
static uint8_t iteration_counter = 0;
static struct timeval tv_start;
static struct timeval tv_end;
static struct timeval tv_dif;
static pthread_t tid;
static uint8_t active_thread = 0;  /* Track if we have an active thread */

/* Global Debug flag */
uint8_t Debug = 0;
/* Global Quiet flag override for lantest (stdout output enabled) */
uint8_t lantest_Quiet = 0;

/* Remote linker names */
CONN *Conn2;
uint16_t VolID;
int PassCount = 0;
int FailCount = 0;
int SkipCount = 0;
int NotTestedCount = 0;
char FailedTests[1024][256] = {{0}};
char NotTestedTests[1024][256] = {{0}};
char SkippedTests[1024][256] = {{0}};

/* Tests configuration */
#define DIRNUM 10  /* 1000 nested dirs */
static int32_t smallfiles = 1000;  /* 1000 files */
static off_t rwsize = 100 * 1024 * 1024;  /* 100 MB */
static int32_t locking = 10000 / 40;  /* 10000 times */
static int32_t create_enum_files = 2000;  /* 2000 files */
static int32_t cache_dirs = 10;  /* dirs for cache tests */
static int32_t cache_files_per_dir = 10;  /* files per dir */
static int32_t cache_lookup_iterations = 100;  /* cache test iterations */
static int32_t mixed_cache_files = 200;  /* mixed ops file count */
static int32_t deep_dir_levels = 20;  /* deep traversal dir levels */
static int32_t deep_test_files = 50;  /* files in deepest dir */
static int32_t deep_traversals = 50;  /* path traversal iterations */
static int32_t validation_files = 100;  /* validation file count */
static int32_t validation_iterations = 100;  /* validation iterations */

/* Forward declarations */
void clean_exit(int exit_code);
static uint64_t numrw;
/* Bool array of which tests to run */
static char teststorun[NUMTESTS];
/* [iteration][test][measurement_type] */
static uint64_t (*results)[][NUMTESTS][NUM_MEASUREMENTS];
static char *bigfilename;

/* Results display control
0 = tabular (default), 1 = CSV */
static int32_t csv_output = 0;
/* Display width for test names in progress output */
#define TEST_NAME_DISPLAY_WIDTH 66

/* Descriptive test names for output formatting with AFP operation counts */
static const char *test_names[NUMTESTS] = {
    "Open, stat and read 512 bytes from 1000 files [8,000 AFP ops]",  /* TEST_OPENSTATREAD */
    "Writing one large file [103 AFP ops]",  /* TEST_WRITE100MB */
    "Reading one large file [102 AFP ops]",  /* TEST_READ100MB */
    "Locking/Unlocking 10000 times each [20,000 AFP ops]",  /* TEST_LOCKUNLOCK */
    "Creating dir with 2000 files [4,000 AFP ops]",  /* TEST_CREATE2000FILES */
    "Enumerate dir with 2000 files [~51 AFP ops]",  /* TEST_ENUM2000FILES */
    "Deleting dir with 2000 files [2,000 AFP ops]",  /* TEST_DELETE2000FILES */
    "Create directory tree with 1000 dirs [1,110 AFP ops]",  /* TEST_CREATEDIR */
    "Directory cache hits (100 dirs + 1000 files) [11,000 AFP ops]",  /* TEST_DIRCACHE_HITS */
    "Mixed cache operations (create/stat/enum/delete) [820 AFP ops]",  /* TEST_DIRCACHE_MIXED */
    "Deep path traversal (nested directory navigation) [3,500 AFP ops]",  /* TEST_DIRCACHE_TRAVERSE */
    "Cache validation efficiency (metadata changes) [30,000 AFP ops]"  /* TEST_CACHE_VALIDATION */
};

/* Global error constants for clean_exit() */
#define ERROR_MEMORY_ALLOCATION  2
#define ERROR_FILE_DIRECTORY_OPS 3
#define ERROR_FORK_OPERATIONS    4
#define ERROR_NETWORK_PROTOCOL   5
#define ERROR_THREAD_OPERATIONS  6
#define ERROR_VALIDATION_TEST    7
#define ERROR_CONFIGURATION      8
#define ERROR_VOLUME_OPERATIONS  9

static void starttimer(void)
{
    gettimeofday(&tv_start, NULL);
}

static void stoptimer(void)
{
    gettimeofday(&tv_end, NULL);
}

static uint64_t timediff(void)
{
    if (tv_end.tv_usec < tv_start.tv_usec) {
        tv_end.tv_usec += 1000000;
        tv_end.tv_sec -= 1;
    }

    tv_dif.tv_sec = tv_end.tv_sec - tv_start.tv_sec;
    tv_dif.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
    return (tv_dif.tv_sec * 1000) + (tv_dif.tv_usec / 1000);
}

/* Helper function to format test name with fixed-width padding */
static void format_padded_test_name(char *dest, const char *src, size_t width)
{
    int32_t len = snprintf(dest, width + 1, "%-*s", (int32_t)width, src);

    if (len >= 0 && len < width) {
        memset(dest + len, ' ', width - len);
    }

    dest[width] = '\0';
}

static void addresult(int32_t test, int32_t iteration)
{
    uint64_t t;
    uint64_t avg;
    t = timediff();
    /* Store timing measurement */
    (*results)[iteration][test][MEASURE_TIME_MS] = t;
#ifdef __linux__
    /* Stop IO monitoring and capture values */
    capture_io_values(TEST_STOP);

    /* Store IO measurements if enabled */
    if (io_monitoring_enabled) {
        (*results)[iteration][test][MEASURE_AFPD_READ_IO] = iodiff_io(afpd_pid, 0);
        (*results)[iteration][test][MEASURE_AFPD_WRITE_IO] = iodiff_io(afpd_pid, 1);
        (*results)[iteration][test][MEASURE_CNID_READ_IO] = iodiff_io(cnid_dbd_pid, 0);
        (*results)[iteration][test][MEASURE_CNID_WRITE_IO] = iodiff_io(cnid_dbd_pid, 1);
    } else {
        (*results)[iteration][test][MEASURE_AFPD_READ_IO] = 0;
        (*results)[iteration][test][MEASURE_AFPD_WRITE_IO] = 0;
        (*results)[iteration][test][MEASURE_CNID_READ_IO] = 0;
        (*results)[iteration][test][MEASURE_CNID_WRITE_IO] = 0;
    }

#else
    /* Non-Linux platforms: set IO measurements to 0 */
    (*results)[iteration][test][MEASURE_AFPD_READ_IO] = 0;
    (*results)[iteration][test][MEASURE_AFPD_WRITE_IO] = 0;
    (*results)[iteration][test][MEASURE_CNID_READ_IO] = 0;
    (*results)[iteration][test][MEASURE_CNID_WRITE_IO] = 0;
#endif
    /* Display human-readable progress */
    char padded_name[TEST_NAME_DISPLAY_WIDTH + 1];
    format_padded_test_name(padded_name, test_names[test], TEST_NAME_DISPLAY_WIDTH);
    fprintf(stdout, "Run %u => %s %6lu ms", iteration + 1, padded_name, t);

    if ((test == TEST_WRITE100MB) || (test == TEST_READ100MB)) {
        if (t > 0) {  /* Prevent division by zero */
            avg = (rwsize / 1000) / t;
            fprintf(stdout, " for %lu MB (avg. %llu MB/s)",
                    rwsize / (1024 * 1024), avg);
        } else {
            fprintf(stdout, " for %lu MB (time too small to measure)",
                    rwsize / (1024 * 1024));
        }
    }

#ifdef __linux__

    /* Display IO measurements if enabled */
    if (io_monitoring_enabled) {
        fprintf(stdout,
                "\n         IO Operations; afpd: %llu READs, %llu WRITEs | cnid_dbd: %llu READs, %llu WRITEs",
                (*results)[iteration][test][MEASURE_AFPD_READ_IO],
                (*results)[iteration][test][MEASURE_AFPD_WRITE_IO],
                (*results)[iteration][test][MEASURE_CNID_READ_IO],
                (*results)[iteration][test][MEASURE_CNID_WRITE_IO]);
    }

#endif
    fprintf(stdout, "\n");
}

/* Helper function to check if measurement should be skipped */
static inline int32_t should_skip_measurement(int32_t measure_type)
{
#ifdef __linux__
    return (!io_monitoring_enabled && measure_type > MEASURE_TIME_MS);
#else
    return (measure_type > MEASURE_TIME_MS);
#endif
}

/* Helper function to calculate statistics for all tests */
static void results_calc_stats(uint64_t
                               averages[NUMTESTS][NUM_MEASUREMENTS],
                               double std_devs[NUMTESTS][NUM_MEASUREMENTS],
                               char teststorun[NUMTESTS])
{
    int32_t i, test, measure_type;
    uint64_t sum;
    int32_t valid_counts[NUMTESTS][NUM_MEASUREMENTS] = {0};

    for (test = 0; test < NUMTESTS; test++) {
        if (!teststorun[test]) {
            continue;
        }

        /* Calculate averages */
        for (measure_type = 0; measure_type < NUM_MEASUREMENTS; measure_type++) {
            if (should_skip_measurement(measure_type)) {
                continue;
            }

            sum = 0;
            valid_counts[test][measure_type] = 0;

            for (i = 0; i < Iterations_save; i++) {
                if ((*results)[i][test][measure_type] > 0) {
                    sum += (*results)[i][test][measure_type];
                    valid_counts[test][measure_type]++;
                }
            }

            if (valid_counts[test][measure_type] > 0) {
                averages[test][measure_type] = sum / valid_counts[test][measure_type];
            }
        }

        /* Calculate standard deviations */
        if (Iterations_save > 1) {
            for (measure_type = 0; measure_type < NUM_MEASUREMENTS; measure_type++) {
                if (should_skip_measurement(measure_type)) {
                    continue;
                }

                if (valid_counts[test][measure_type] > 1) {
                    double sum_sq_diff = 0.0;

                    for (i = 0; i < Iterations_save; i++) {
                        if ((*results)[i][test][measure_type] > 0) {
                            double diff = (double)((*results)[i][test][measure_type]) -
                                          (double)averages[test][measure_type];
                            sum_sq_diff += diff * diff;
                        }
                    }

                    std_devs[test][measure_type] = sqrt(sum_sq_diff /
                                                        (valid_counts[test][measure_type] - 1));
                }
            }
        }
    }
}

/* Helper function to print headers */
static void results_print_headers(int32_t is_csv)
{
#ifdef __linux__

    if (io_monitoring_enabled) {
        if (is_csv) {
            fprintf(stdout,
                    "Test,Time_ms,Time±,AFPD_R,AFPD_R±,AFPD_W,AFPD_W±,CNID_R,CNID_R±,CNID_W,CNID_W±,MB/s\n");
        } else {
            fprintf(stdout, "%-66s %8s %6s %6s %7s %6s %7s %6s %7s %6s %7s %6s\n",
                    "Test", " Time_ms", " Time±", "AFPD_R", "AFPD_R±", "AFPD_W", "AFPD_W±",
                    "CNID_R", "CNID_R±", "CNID_W", "CNID_W±", "MB/s");
            fprintf(stdout, "%-66s %8s %6s %6s %7s %6s %7s %6s %7s %6s %7s %6s\n",
                    "------------------------------------------------------------------",
                    "--------", "------",
                    "------", "-------", "------", "-------", "------", "-------", "------",
                    "-------", "------");
        }
    } else {
        if (is_csv) {
            fprintf(stdout, "Test,Time_ms,Time±,MB/s\n");
        } else {
            fprintf(stdout, "%-66s %7s %6s %6s\n",
                    "Test", "Time_ms", "Time±", "MB/s");
            fprintf(stdout, "%-66s %7s %6s %6s\n",
                    "------------------------------------------------------------------", "-------",
                    "------",
                    "------");
        }
    }

#else

    if (is_csv) {
        fprintf(stdout, "Test,Time_ms,Time±,MB/s\n");
    } else {
        fprintf(stdout, "%-66s %7s %6s %6s\n",
                "Test", "Time_ms", "Time±", "MB/s");
        fprintf(stdout, "%-66s %7s %6s %6s\n",
                "------------------------------------------------------------------",
                "--------", "------",
                "------");
    }

#endif
}

/* Helper function to print data row */
static void results_print_row(int32_t test, int32_t is_csv,
                              uint64_t averages[NUMTESTS][NUM_MEASUREMENTS],
                              double std_devs[NUMTESTS][NUM_MEASUREMENTS])
{
    /* Calculate throughput for file I/O tests */
    uint64_t thrput = 0;

    if ((test == TEST_WRITE100MB || test == TEST_READ100MB)
            && averages[test][MEASURE_TIME_MS] > 0) {
        thrput = (100 * 1000) / averages[test][MEASURE_TIME_MS];
    }

    /* Print test identifier */
    const char *test_id = is_csv ? "Test_%d" : test_names[test];
#ifdef __linux__

    if (io_monitoring_enabled) {
        if (is_csv) {
            fprintf(stdout,
                    "%s,%llu,%.1f,%llu,%.1f,%llu,%.1f,%llu,%.1f,%llu,%.1f,%llu\n",
                    test_names[test],
                    averages[test][MEASURE_TIME_MS], std_devs[test][MEASURE_TIME_MS],
                    averages[test][MEASURE_AFPD_READ_IO], std_devs[test][MEASURE_AFPD_READ_IO],
                    averages[test][MEASURE_AFPD_WRITE_IO], std_devs[test][MEASURE_AFPD_WRITE_IO],
                    averages[test][MEASURE_CNID_READ_IO], std_devs[test][MEASURE_CNID_READ_IO],
                    averages[test][MEASURE_CNID_WRITE_IO], std_devs[test][MEASURE_CNID_WRITE_IO],
                    thrput);
        } else {
            fprintf(stdout,
                    "%-66s %8llu %6.1f %6llu %7.1f %6llu %7.1f %6llu %7.1f %6llu %7.1f %6llu\n",
                    test_names[test],
                    averages[test][MEASURE_TIME_MS], std_devs[test][MEASURE_TIME_MS],
                    averages[test][MEASURE_AFPD_READ_IO], std_devs[test][MEASURE_AFPD_READ_IO],
                    averages[test][MEASURE_AFPD_WRITE_IO], std_devs[test][MEASURE_AFPD_WRITE_IO],
                    averages[test][MEASURE_CNID_READ_IO], std_devs[test][MEASURE_CNID_READ_IO],
                    averages[test][MEASURE_CNID_WRITE_IO], std_devs[test][MEASURE_CNID_WRITE_IO],
                    thrput);
        }
    } else {
        if (is_csv) {
            fprintf(stdout, "%s,%llu,%.1f,%llu\n",
                    test_names[test],
                    averages[test][MEASURE_TIME_MS], std_devs[test][MEASURE_TIME_MS],
                    thrput);
        } else {
            fprintf(stdout, "%-66s %7llu %6.1f %6llu\n",
                    test_names[test],
                    averages[test][MEASURE_TIME_MS], std_devs[test][MEASURE_TIME_MS],
                    thrput);
        }
    }

#else

    if (is_csv) {
        fprintf(stdout, "%s,%llu,%.1f,%llu\n",
                test_names[test],
                averages[test][MEASURE_TIME_MS], std_devs[test][MEASURE_TIME_MS],
                thrput);
    } else {
        fprintf(stdout, "%-66s %7llu %6.1f %6llu\n",
                test_names[test],
                averages[test][MEASURE_TIME_MS], std_devs[test][MEASURE_TIME_MS],
                thrput);
    }

#endif
}

static void result_print_summary(uint64_t
                                 averages[NUMTESTS][NUM_MEASUREMENTS],
                                 int32_t csv_output,
                                 char teststorun[NUMTESTS])
{
    int32_t test, measure_type;
    double column_sums[NUM_MEASUREMENTS] = {0.0};

    /* Calculate sums for each measurement type across all active tests */
    for (test = 0; test < NUMTESTS; test++) {
        if (!teststorun[test]) {
            continue;
        }

        for (measure_type = 0; measure_type < NUM_MEASUREMENTS; measure_type++) {
            if (should_skip_measurement(measure_type)) {
                continue;
            }

            column_sums[measure_type] += averages[test][measure_type];
        }
    }

    /* Print separator line */
    if (!csv_output) {
#ifdef __linux__

        if (io_monitoring_enabled) {
            fprintf(stdout, "%-66s %8s %6s %6s %7s %6s %7s %6s %7s %6s %7s %6s\n",
                    "------------------------------------------------------------------",
                    "--------", "------",
                    "------", "-------", "------", "-------", "------", "-------", "------",
                    "-------", "------");
        } else {
            fprintf(stdout, "%-66s %7s %6s %6s\n",
                    "------------------------------------------------------------------", "-------",
                    "------",
                    "------");
        }

#else
        fprintf(stdout, "%-66s %7s %6s %6s\n",
                "------------------------------------------------------------------", "-------",
                "------",
                "------");
#endif
    }

    /* Print summary row */
#ifdef __linux__

    if (io_monitoring_enabled) {
        if (csv_output) {
            fprintf(stdout,
                    "Sum of all AFP OPs = %d,%.0f,,%.0f,,%.0f,,%.0f,,%.0f,,\n",
                    TOTAL_AFP_OPS,
                    column_sums[MEASURE_TIME_MS],
                    column_sums[MEASURE_AFPD_READ_IO],
                    column_sums[MEASURE_AFPD_WRITE_IO],
                    column_sums[MEASURE_CNID_READ_IO],
                    column_sums[MEASURE_CNID_WRITE_IO]);
        } else {
            char summary_str[100];
            snprintf(summary_str, sizeof(summary_str), "Sum of all AFP OPs = %d",
                     TOTAL_AFP_OPS);
            fprintf(stdout,
                    "%-66s %8.0f %6s %6.0f %7s %6.0f %7s %6.0f %7s %6.0f %7s %6s\n",
                    summary_str,
                    column_sums[MEASURE_TIME_MS], "",
                    column_sums[MEASURE_AFPD_READ_IO], "",
                    column_sums[MEASURE_AFPD_WRITE_IO], "",
                    column_sums[MEASURE_CNID_READ_IO], "",
                    column_sums[MEASURE_CNID_WRITE_IO], "",
                    "");
        }
    } else {
        if (csv_output) {
            fprintf(stdout, "Sum of all AFP OPs = %d,%.0f,,\n",
                    TOTAL_AFP_OPS,
                    column_sums[MEASURE_TIME_MS]);
        } else {
            char summary_str[100];
            snprintf(summary_str, sizeof(summary_str), "Sum of all AFP OPs = %d",
                     TOTAL_AFP_OPS);
            fprintf(stdout, "%-66s %7.0f %6s %6s\n",
                    summary_str,
                    column_sums[MEASURE_TIME_MS], "", "");
        }
    }

#else

    if (csv_output) {
        fprintf(stdout, "Sum of all AFP OPs = %d,%.0f,,\n",
                TOTAL_AFP_OPS,
                column_sums[MEASURE_TIME_MS]);
    } else {
        char summary_str[100];
        snprintf(summary_str, sizeof(summary_str), "Sum of all AFP OPs = %d",
                 TOTAL_AFP_OPS);
        fprintf(stdout, "%-66s %7.0f %6s %6s\n",
                summary_str,
                column_sums[MEASURE_TIME_MS], "", "");
    }

#endif

    /* Print aggregates summary */
    if (!csv_output) {
        fprintf(stdout, "\nAggregates Summary:\n");
        fprintf(stdout, "-------------------\n");
        /* Calculate average time per AFP OP */
        double avg_time_per_op = column_sums[MEASURE_TIME_MS] / (double)TOTAL_AFP_OPS;
        fprintf(stdout, "Average Time per AFP OP: %.3f ms\n", avg_time_per_op);
#ifdef __linux__

        if (io_monitoring_enabled) {
            /* Calculate ratio of AFPD Reads to all AFP OPs */
            double read_ratio = column_sums[MEASURE_AFPD_READ_IO] / (double)TOTAL_AFP_OPS;
            fprintf(stdout, "Average AFPD Reads per AFP OP: %.3f\n", read_ratio);
            /* Calculate ratio of AFPD Writes to all AFP OPs */
            double write_ratio = column_sums[MEASURE_AFPD_WRITE_IO] / (double)TOTAL_AFP_OPS;
            fprintf(stdout, "Average AFPD Writes per AFP OP: %.3f\n", write_ratio);
        }

#endif
        fprintf(stdout, "\n");
    }
}

static void displayresults(void)
{
    int32_t i, test, maxindex, minindex;
    uint64_t sum;
    uint64_t max, min;
    int32_t measure_type;

    /* Eliminate runaways for all measurement types */
    if (Iterations_save > 5) {
        for (test = 0; test != NUMTESTS; test++) {
            if (!teststorun[test]) {
                continue;
            }

            /* Eliminate outliers for each measurement type */
            for (measure_type = 0; measure_type < NUM_MEASUREMENTS; measure_type++) {
                if (should_skip_measurement(measure_type)) {
                    continue;
                }

                max = 0;
                min = ULONG_MAX;
                maxindex = 0;
                minindex = 0;

                /* Find min/max for this measurement type */
                for (i = 0; i < Iterations_save; i++) {
                    if ((*results)[i][test][measure_type] < min) {
                        min = (*results)[i][test][measure_type];
                        minindex = i;
                    }

                    if ((*results)[i][test][measure_type] > max) {
                        max = (*results)[i][test][measure_type];
                        maxindex = i;
                    }
                }

                /* Exclude outliers */
                (*results)[minindex][test][measure_type] = 0;
                (*results)[maxindex][test][measure_type] = 0;
            }
        }
    }

    /* Calculate statistics (results and standard deviations) for each test */
    uint64_t averages[NUMTESTS][NUM_MEASUREMENTS] = {0};
    double std_devs[NUMTESTS][NUM_MEASUREMENTS] = {0.0};
    results_calc_stats(averages, std_devs, teststorun);
    /* Display results banner */
    fprintf(stdout, "\nNetatalk Lantest Results");

    if (Iterations_save > 1) {
        fprintf(stdout,
                " (Averages and standard deviations (±) for all tests, across %d iterations%s)",
                Iterations_save,
                Iterations_save == 2 ? " (default)" : "");
    } else {
        fprintf(stdout, " (single iteration)");
    }

    fprintf(stdout, "\n");
    fprintf(stdout,
            "============================================================================================================\n\n");
    /* Print results headers based on output format */
    results_print_headers(csv_output);

    /* Output row data for each test */
    for (test = 0; test < NUMTESTS; test++) {
        if (!teststorun[test]) {
            continue;
        }

        results_print_row(test, csv_output, averages, std_devs);
    }

    /* Add summary row showing sum of all columns */
    result_print_summary(averages, csv_output, teststorun);
    /* TODO If running on Localhost with access to afpd.logs, print 'dircache statistics:' log line */
}

/* Safe integer conversion with validation */
static int32_t safe_atoi(const char *str, const char *param_name,
                         int32_t min_val,
                         int32_t max_val)
{
    char *endptr;
    int64_t val;

    if (!str || *str == '\0') {
        fprintf(stderr, "Error: Empty %s parameter\n", param_name);
        clean_exit(ERROR_CONFIGURATION);
    }

    errno = 0;  /* Reset errno before conversion */
    val = strtol(str, &endptr, 10);

    /* Check for conversion errors */
    if (errno == ERANGE || val < LONG_MIN || val > LONG_MAX) {
        fprintf(stderr, "Error: %s parameter '%s' out of range\n", param_name, str);
        clean_exit(ERROR_CONFIGURATION);
    }

    /* Check for non-numeric characters */
    if (*endptr != '\0') {
        fprintf(stderr,
                "Error: Invalid %s parameter '%s' - contains non-numeric characters\n",
                param_name, str);
        clean_exit(ERROR_CONFIGURATION);
    }

    /* Check parameter-specific bounds */
    if (val < min_val || val > max_val) {
        fprintf(stderr, "Error: %s parameter %ld out of valid range [%d, %d]\n",
                param_name, val, min_val, max_val);
        clean_exit(ERROR_CONFIGURATION);
    }

    return (int32_t)val;
}

/* ------------------------- */
void clean_exit(int exit_code)
{
    /* Only print error details if this is an error exit (exit_code > 0) */
    if (exit_code > 0) {
        fprintf(stderr, "\n\tERROR (Exit Code: %d)\n", exit_code);

        /* Print error description based on exit code */
        switch (exit_code) {
        case ERROR_MEMORY_ALLOCATION:
            fprintf(stderr, "\tDescription: Buffer/cache allocation failure\n");
            break;

        case ERROR_FILE_DIRECTORY_OPS:
            fprintf(stderr, "\tDescription: File/Directory Create/delete/access failure\n");
            break;

        case ERROR_FORK_OPERATIONS:
            fprintf(stderr, "\tDescription: Fork open/close failure\n");
            break;

        case ERROR_NETWORK_PROTOCOL:
            fprintf(stderr, "\tDescription: Network/AFP protocol read/write error\n");
            break;

        case ERROR_THREAD_OPERATIONS:
            fprintf(stderr, "\tDescription: Thread Operation failure\n");
            break;

        case ERROR_VALIDATION_TEST:
            fprintf(stderr, "\tDescription: Test validation error\n");
            break;

        case ERROR_CONFIGURATION:
            fprintf(stderr, "\tDescription: Invalid config parameter\n");
            break;

        case ERROR_VOLUME_OPERATIONS:
            fprintf(stderr, "\tDescription: Volume open/access failure\n");
            break;

        default:
            fprintf(stderr, "\tDescription: Unspecified error\n");
            break;
        }

        fprintf(stderr, "\n");
    }

    /* Thread cleanup - attempt to join active thread if it exists */
    if (active_thread) {
        fprintf(stderr, "Attempting to clean up active thread...\n");

        if (pthread_join(tid, NULL) != 0) {
            fprintf(stderr, "Warning: Failed to join thread during cleanup\n");
            pthread_cancel(tid);
        }

        active_thread = 0;
    }

    /* Perform cleanup to prevent resource leaks, AFP Session, Sockets, Connection, and struts */
    if (Conn) {
        FPLogOut(Conn);

        /* Always close socket regardless of logout success */
        if (Conn->dsi.socket >= 0) {
            CloseClientSocket(Conn->dsi.socket);
            Conn->dsi.socket = -1;
        }

        free(Conn);
        Conn = NULL;
    }

    if (results) {
        free(results);
        results = NULL;
    }

    if (bigfilename) {
        free(bigfilename);
        bigfilename = NULL;
    }

    exit(exit_code);
}

/* --------------------------------- */
int32_t is_there(CONN *conn, uint16_t vol, int32_t did, char *name)
{
    return FPGetFileDirParams(conn, vol, did, name,
                              (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID)
                              ,
                              (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID)
                             );
}

struct async_io_req {
    uint64_t air_count;
    size_t air_size;
};

static void *rply_thread(void *p)
{
    struct async_io_req *air = p;
    size_t size = air->air_size;
    uint64_t n = air->air_count;
    size_t stored;
    ssize_t len;
    char *buf = NULL;
    buf = malloc(size);

    if (!buf) {
        fprintf(stderr, "Memory allocation failed for DSI buffer (%zu bytes)\n", size);
        clean_exit(ERROR_MEMORY_ALLOCATION);
    }

    while (n--) {
        stored = 0;

        while (stored < size) {
            if ((len = recv(dsi->socket, (uint8_t *)buf + stored, size - stored, 0)) == -1
                    && errno == EINTR) {
                continue;
            }

            if (len > 0) {
                stored += len;
            } else {
                fprintf(stderr, "dsi_stream_read(%ld): %s\n", len,
                        (len < 0) ? strerror(errno) : "EOF");
                goto exit;
            }
        }
    }

exit:

    if (buf) {
        free(buf);
    }

    return NULL;
}

/* ------------------------- */
void run_test(const int32_t dir)
{
    static char *data;
    int32_t i, maxi = 0;
    int32_t j, k;
    int32_t fork;
    int32_t test = 0;
    int32_t nowrite;
    off_t offset;
    struct async_io_req air;
    static char temp[MAXPATHLEN];
    numrw = rwsize / (dsi->server_quantum - FPWRITE_RQST_SIZE);

    if (!data) {
        data = calloc(1, dsi->server_quantum);

        if (!data) {
            fprintf(stderr, "Memory allocation failed for data buffer\n");
            clean_exit(ERROR_MEMORY_ALLOCATION);
        }
    }

    /* --------------- */
    /* Test (1)        */
    if (teststorun[TEST_OPENSTATREAD]) {
        /* Create files before test */
        for (i = 0; i < smallfiles; i++) {
            snprintf(temp, sizeof(temp), "File.small%d", i);

            if (ntohl(AFPERR_NOOBJ) != is_there(Conn, vol, dir, temp)) {
                clean_exit(ERROR_FILE_DIRECTORY_OPS);
            }

            if (FPGetFileDirParams(Conn, vol, dir, "", 0, (1 << DIRPBIT_DID))) {
                clean_exit(ERROR_FILE_DIRECTORY_OPS);
            }

            if (FPCreateFile(Conn, vol, 0, dir, temp)) {
                clean_exit(ERROR_FILE_DIRECTORY_OPS);
            }

            if (is_there(Conn, vol, dir, temp)) {
                clean_exit(ERROR_FILE_DIRECTORY_OPS);
            }

            if (FPGetFileDirParams(Conn, vol, dir, temp,
                                   (1 << FILPBIT_FNUM) | (1 << FILPBIT_PDID) | (1 << FILPBIT_FINFO) |
                                   (1 << FILPBIT_CDATE) | (1 << FILPBIT_MDATE) | (1 << FILPBIT_DFLEN) |
                                   (1 << FILPBIT_RFLEN)
                                   , 0)) {
                clean_exit(ERROR_FILE_DIRECTORY_OPS);
            }

            if (FPGetFileDirParams(Conn, vol, dir, temp,
                                   (1 << FILPBIT_FNUM) | (1 << FILPBIT_PDID) | (1 << FILPBIT_FINFO) |
                                   (1 << DIRPBIT_ATTR) | (1 << DIRPBIT_BDATE) |
                                   (1 << FILPBIT_CDATE) | (1 << FILPBIT_MDATE) | (1 << FILPBIT_DFLEN) |
                                   (1 << FILPBIT_RFLEN)
                                   , 0)) {
                clean_exit(ERROR_FILE_DIRECTORY_OPS);
            }

            fork = FPOpenFork(Conn, vol, OPENFORK_DATA,
                              (1 << FILPBIT_PDID) | (1 << DIRPBIT_LNAME) | (1 << FILPBIT_FNUM) |
                              (1 << FILPBIT_DFLEN)
                              , dir, temp, OPENACC_WR | OPENACC_RD | OPENACC_DWR | OPENACC_DRD);

            if (!fork) {
                clean_exit(ERROR_FORK_OPERATIONS);
            }

            if (FPGetForkParam(Conn, fork,
                               (1 << FILPBIT_PDID) | (1 << DIRPBIT_LNAME) | (1 << FILPBIT_DFLEN))) {
                clean_exit(ERROR_FORK_OPERATIONS);
            }

            if (FPWrite(Conn, fork, 0, 20480, data, 0)) {
                clean_exit(ERROR_NETWORK_PROTOCOL);
            }

            if (FPCloseFork(Conn, fork)) {
                clean_exit(ERROR_NETWORK_PROTOCOL);
            }

            maxi = i;
        }

        if (FPEnumerate(Conn, vol, dir, "",
                        (1 << FILPBIT_LNAME) | (1 << FILPBIT_FNUM) | (1 << FILPBIT_ATTR) |
                        (1 << FILPBIT_FINFO) |
                        (1 << FILPBIT_CDATE) | (1 << FILPBIT_BDATE) | (1 << FILPBIT_MDATE) |
                        (1 << FILPBIT_DFLEN) | (1 << FILPBIT_RFLEN)
                        ,
                        (1 << DIRPBIT_ATTR) | (1 << DIRPBIT_ATTR) | (1 << DIRPBIT_FINFO) |
                        (1 << DIRPBIT_CDATE) | (1 << DIRPBIT_BDATE) | (1 << DIRPBIT_MDATE) |
                        (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
                        (1 << DIRPBIT_ACCESS)
                       )) {
            clean_exit(ERROR_FILE_DIRECTORY_OPS);
        }

#ifdef __linux__
        capture_io_values(TEST_START);
#endif
        starttimer();

        /* Read files for test - OPTIMIZED to reduce AFP operations */
        for (i = 0; i <= maxi; i++) {
            snprintf(temp, sizeof(temp), "File.small%d", i);

            /* Stat operations first (3 AFP ops) */
            if (is_there(Conn, vol, dir, temp)) {
                clean_exit(ERROR_FILE_DIRECTORY_OPS);
            }

            if (FPGetFileDirParams(Conn, vol, dir, temp, 0x72d, 0)) {
                clean_exit(ERROR_FILE_DIRECTORY_OPS);
            }

            if (FPGetFileDirParams(Conn, vol, dir, temp, 0x73f, 0x133f)) {
                clean_exit(ERROR_FILE_DIRECTORY_OPS);
            }

            /* Open once with read+write permissions (1 AFP op) */
            fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0x342, dir, temp,
                              OPENACC_RD | OPENACC_DWR);

            if (!fork) {
                clean_exit(ERROR_FORK_OPERATIONS);
            }

            /* Get fork params (2 AFP ops) */
            if (FPGetForkParam(Conn, fork, (1 << FILPBIT_DFLEN))) {
                clean_exit(ERROR_FORK_OPERATIONS);
            }

            if (FPGetForkParam(Conn, fork, 0x242)) {
                clean_exit(ERROR_FORK_OPERATIONS);
            }

            /* Read operation (1 AFP op) */
            if (FPRead(Conn, fork, 0, 512, data)) {
                clean_exit(ERROR_NETWORK_PROTOCOL);
            }

            /* Close fork (1 AFP op) */
            if (FPCloseFork(Conn, fork)) {
                clean_exit(ERROR_NETWORK_PROTOCOL);
            }
        }

        stoptimer();
        addresult(TEST_OPENSTATREAD, iteration_counter);

        /* Clean up files after test */
        for (i = 0; i <= maxi; i++) {
            snprintf(temp, sizeof(temp), "File.small%d", i);

            if (is_there(Conn, vol, dir, temp)) {
                clean_exit(ERROR_FILE_DIRECTORY_OPS);
            }

            if (FPGetFileDirParams(Conn, vol, dir, temp, 0, (1 << FILPBIT_FNUM))) {
                clean_exit(ERROR_FILE_DIRECTORY_OPS);
            }

            if (FPDelete(Conn, vol, dir, temp)) {
                clean_exit(ERROR_FILE_DIRECTORY_OPS);
            }
        }

        if (FPGetVolParam(Conn, vol, (1 << VOLPBIT_MDATE) | (1 << VOLPBIT_XBFREE))) {
            clean_exit(ERROR_NETWORK_PROTOCOL);
        }
    }

    /* -------- */
    /* Test (2) */
    if (teststorun[TEST_WRITE100MB]) {
        snprintf(temp, sizeof(temp), "File.big");

        if (FPCreateFile(Conn, vol, 0, dir, temp)) {
            clean_exit(ERROR_FILE_DIRECTORY_OPS);
        }

        if (FPGetFileDirParams(Conn, vol, dir, temp, 0x72d, 0)) {
            clean_exit(ERROR_FILE_DIRECTORY_OPS);
        }

        if (FPGetFileDirParams(Conn, vol, dir, temp, 0x73f, 0x133f)) {
            clean_exit(ERROR_FILE_DIRECTORY_OPS);
        }

        fork = FPOpenFork(Conn, vol, OPENFORK_DATA,
                          (1 << FILPBIT_PDID) | (1 << FILPBIT_FNUM) | (1 << FILPBIT_DFLEN)
                          , dir, temp, OPENACC_WR | OPENACC_RD | OPENACC_DWR | OPENACC_DRD);

        if (!fork) {
            clean_exit(ERROR_FORK_OPERATIONS);
        }

        if (FPGetForkParam(Conn, fork, (1 << FILPBIT_PDID))) {
            clean_exit(ERROR_FORK_OPERATIONS);
        }

        air.air_count = numrw;
        air.air_size = FPWRITE_RPLY_SIZE;
        int32_t pthread_ret = pthread_create(&tid, NULL, rply_thread, &air);

        if (pthread_ret != 0) {
            fprintf(stderr, "pthread_create failed: %d\n", pthread_ret);
            clean_exit(ERROR_THREAD_OPERATIONS);
        }

        active_thread = 1;  /* Mark thread as active */
#ifdef __linux__
        capture_io_values(TEST_START);
#endif
        starttimer();

        for (i = 0, offset = 0; i < numrw;
                offset += (dsi->server_quantum - FPWRITE_RQST_SIZE), i++) {
            if (FPWrite_ext_async(Conn, fork, offset,
                                  dsi->server_quantum - FPWRITE_RQST_SIZE, data, 0)) {
                clean_exit(ERROR_NETWORK_PROTOCOL);
            }
        }

        pthread_ret = pthread_join(tid, NULL);

        if (pthread_ret != 0) {
            fprintf(stderr, "pthread_join failed: %d\n", pthread_ret);
            /* Thread join failed, but thread may still be running */
            /* Try pthread_cancel as cleanup attempt before fatal exit */
            pthread_cancel(tid);
            clean_exit(ERROR_THREAD_OPERATIONS);
        }

        active_thread = 0;  /* Thread successfully joined */

        if (FPCloseFork(Conn, fork)) {
            clean_exit(ERROR_NETWORK_PROTOCOL);
        }

        stoptimer();
        addresult(TEST_WRITE100MB, iteration_counter);
    }

    /* -------- */
    /* Test (3) */
    if (teststorun[TEST_READ100MB]) {
        if (!bigfilename) {
            if (is_there(Conn, vol, dir, temp) != AFP_OK) {
                clean_exit(ERROR_FILE_DIRECTORY_OPS);
            }

            fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0x342, dir, temp,
                              OPENACC_RD | OPENACC_DWR);
        } else {
            if (is_there(Conn, vol, DIRDID_ROOT, bigfilename) != AFP_OK) {
                clean_exit(ERROR_FILE_DIRECTORY_OPS);
            }

            fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0x342, DIRDID_ROOT, bigfilename,
                              OPENACC_RD | OPENACC_DWR);
        }

        if (!fork) {
            clean_exit(ERROR_FORK_OPERATIONS);
        }

        if (FPGetForkParam(Conn, fork, 0x242)) {
            clean_exit(ERROR_FORK_OPERATIONS);
        }

        air.air_count = numrw;
        air.air_size = (dsi->server_quantum - FPWRITE_RQST_SIZE) + 16;
        int32_t pthread_ret = pthread_create(&tid, NULL, rply_thread, &air);

        if (pthread_ret != 0) {
            fprintf(stderr, "pthread_create failed: %d\n", pthread_ret);
            clean_exit(ERROR_THREAD_OPERATIONS);
        }

        active_thread = 1;  /* Mark thread as active */
#ifdef __linux__
        capture_io_values(TEST_START);
#endif
        starttimer();

        for (i = 0; i < numrw ; i++) {
            if (FPRead_ext_async(Conn, fork, i * (dsi->server_quantum - FPWRITE_RQST_SIZE),
                                 dsi->server_quantum - FPWRITE_RQST_SIZE, data)) {
                clean_exit(ERROR_NETWORK_PROTOCOL);
            }
        }

        pthread_ret = pthread_join(tid, NULL);

        if (pthread_ret != 0) {
            fprintf(stderr, "pthread_join failed: %d\n", pthread_ret);
            /* Thread join failed, but thread may still be running */
            /* Try pthread_cancel as cleanup attempt before fatal exit */
            pthread_cancel(tid);
            clean_exit(ERROR_THREAD_OPERATIONS);
        }

        active_thread = 0;  /* Thread successfully joined */

        if (FPCloseFork(Conn, fork)) {
            clean_exit(ERROR_NETWORK_PROTOCOL);
        }

        stoptimer();
        addresult(TEST_READ100MB, iteration_counter);
    }

    /* Remove test 2+3 testfile */
    if (teststorun[TEST_WRITE100MB]) {
        FPDelete(Conn, vol, dir, "File.big");
    }

    /* -------- */
    /* Test (4) */
    if (teststorun[TEST_LOCKUNLOCK]) {
        snprintf(temp, sizeof(temp), "File.lock");

        if (ntohl(AFPERR_NOOBJ) != is_there(Conn, vol, dir, temp)) {
            clean_exit(ERROR_FILE_DIRECTORY_OPS);
        }

        if (FPGetFileDirParams(Conn, vol, dir, "", 0, (1 << DIRPBIT_DID))) {
            clean_exit(ERROR_FILE_DIRECTORY_OPS);
        }

        if (FPCreateFile(Conn, vol, 0, dir, temp)) {
            clean_exit(ERROR_FILE_DIRECTORY_OPS);
        }

        if (is_there(Conn, vol, dir, temp)) {
            clean_exit(ERROR_FILE_DIRECTORY_OPS);
        }

        if (FPGetFileDirParams(Conn, vol, dir, temp,
                               (1 << FILPBIT_FNUM) | (1 << FILPBIT_PDID) | (1 << FILPBIT_FINFO) |
                               (1 << FILPBIT_CDATE) | (1 << FILPBIT_DFLEN) | (1 << FILPBIT_RFLEN)
                               , 0)) {
            clean_exit(ERROR_FILE_DIRECTORY_OPS);
        }

        fork = FPOpenFork(Conn, vol, OPENFORK_DATA,
                          (1 << FILPBIT_PDID) | (1 << FILPBIT_FNUM) | (1 << FILPBIT_DFLEN)
                          , dir, temp, OPENACC_WR | OPENACC_RD | OPENACC_DWR | OPENACC_DRD);

        if (!fork) {
            clean_exit(ERROR_FORK_OPERATIONS);
        }

        if (FPGetForkParam(Conn, fork, (1 << FILPBIT_PDID) | (1 << FILPBIT_DFLEN))) {
            clean_exit(ERROR_FORK_OPERATIONS);
        }

        if (FPGetFileDirParams(Conn, vol, dir, temp,
                               (1 << DIRPBIT_ATTR) | (1 << FILPBIT_CDATE) | (1 << FILPBIT_MDATE) |
                               (1 << FILPBIT_FNUM) |
                               (1 << FILPBIT_FINFO) | (1 << FILPBIT_DFLEN) | (1 << FILPBIT_RFLEN)
                               , 0)) {
            clean_exit(ERROR_FILE_DIRECTORY_OPS);
        }

        if (FPWrite(Conn, fork, 0, 40000, data, 0)) {
            clean_exit(ERROR_NETWORK_PROTOCOL);
        }

        if (FPCloseFork(Conn, fork)) {
            clean_exit(ERROR_NETWORK_PROTOCOL);
        }

        if (is_there(Conn, vol, dir, temp)) {
            clean_exit(ERROR_FILE_DIRECTORY_OPS);
        }

        if (FPGetFileDirParams(Conn, vol, dir, temp, 0x73f, 0x133f)) {
            clean_exit(ERROR_FILE_DIRECTORY_OPS);
        }

        fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0x342, dir, temp, OPENACC_RD);

        if (!fork) {
            clean_exit(ERROR_FORK_OPERATIONS);
        }

        if (FPGetForkParam(Conn, fork, (1 << FILPBIT_DFLEN))) {
            clean_exit(ERROR_FORK_OPERATIONS);
        }

        if (FPRead(Conn, fork, 0, 512, data)) {
            clean_exit(ERROR_NETWORK_PROTOCOL);
        }

        if (FPCloseFork(Conn, fork)) {
            clean_exit(ERROR_NETWORK_PROTOCOL);
        }

        fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0x342, dir, temp,
                          OPENACC_RD | OPENACC_WR);

        if (!fork) {
            clean_exit(ERROR_FORK_OPERATIONS);
        }

        if (FPGetForkParam(Conn, fork, 0x242)) {
            clean_exit(ERROR_FORK_OPERATIONS);
        }

        if (FPGetFileDirParams(Conn, vol, dir, temp, 0x72d, 0)) {
            clean_exit(ERROR_FILE_DIRECTORY_OPS);
        }

#ifdef __linux__
        capture_io_values(TEST_START);
#endif
        starttimer();

        for (j = 0; j < locking; j++) {
            for (i = 0; i <= 390; i += 10) {
                if (FPByteLock(Conn, fork, 0, 0, i, 10)) {
                    clean_exit(ERROR_NETWORK_PROTOCOL);
                }

                if (FPByteLock(Conn, fork, 0, 1, i, 10)) {
                    clean_exit(ERROR_NETWORK_PROTOCOL);
                }
            }
        }

        stoptimer();
        addresult(TEST_LOCKUNLOCK, iteration_counter);

        if (is_there(Conn, vol, dir, temp)) {
            clean_exit(ERROR_FILE_DIRECTORY_OPS);
        }

        if (FPCloseFork(Conn, fork)) {
            clean_exit(ERROR_NETWORK_PROTOCOL);
        }

        if (FPDelete(Conn, vol, dir, "File.lock")) {
            clean_exit(ERROR_FILE_DIRECTORY_OPS);
        }
    }

    /* -------- */
    /* Test (5) */
    if (teststorun[TEST_CREATE2000FILES]) {
#ifdef __linux__
        capture_io_values(TEST_START);
#endif
        starttimer();

        for (i = 1; i <= create_enum_files; i++) {
            snprintf(temp, sizeof(temp), "File.0k%d", i);

            if (FPCreateFile(Conn, vol, 0, dir, temp)) {
                clean_exit(ERROR_FILE_DIRECTORY_OPS);
                break;
            }

            if (FPGetFileDirParams(Conn, vol, dir, temp,
                                   (1 << FILPBIT_FNUM) | (1 << FILPBIT_PDID) | (1 << FILPBIT_FINFO) |
                                   (1 << FILPBIT_CDATE) | (1 << FILPBIT_DFLEN) | (1 << FILPBIT_RFLEN)
                                   , 0) != AFP_OK) {
                clean_exit(ERROR_FILE_DIRECTORY_OPS);
            }

            maxi = i;  /* Track last successful file creation */
        }

        stoptimer();
        addresult(TEST_CREATE2000FILES, iteration_counter);
    }

    /* -------- */
    /* Test (6) */
    if (teststorun[TEST_ENUM2000FILES]) {
#ifdef __linux__
        capture_io_values(TEST_START);
#endif
        starttimer();

        if (FPEnumerateFull(Conn, vol, 1, 40, DSI_DATASIZ, dir, "",
                            (1 << FILPBIT_LNAME) | (1 << FILPBIT_FNUM) | (1 << FILPBIT_ATTR) |
                            (1 << FILPBIT_FINFO) | (1 << FILPBIT_CDATE) | (1 << FILPBIT_BDATE) |
                            (1 << FILPBIT_MDATE) | (1 << FILPBIT_DFLEN) | (1 << FILPBIT_RFLEN)
                            ,
                            (1 << DIRPBIT_ATTR) | (1 << DIRPBIT_ATTR) | (1 << DIRPBIT_FINFO) |
                            (1 << DIRPBIT_CDATE) | (1 << DIRPBIT_BDATE) | (1 << DIRPBIT_MDATE) |
                            (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
                            (1 << DIRPBIT_ACCESS))) {
            clean_exit(ERROR_NETWORK_PROTOCOL);
        }

        for (i = 41; (i + 40) < create_enum_files; i += 80) {
            if (FPEnumerateFull(Conn, vol, i + 40, 40, DSI_DATASIZ, dir, "",
                                (1 << FILPBIT_LNAME) | (1 << FILPBIT_FNUM) | (1 << FILPBIT_ATTR) |
                                (1 << FILPBIT_FINFO) | (1 << FILPBIT_CDATE) | (1 << FILPBIT_BDATE) |
                                (1 << FILPBIT_MDATE) | (1 << FILPBIT_DFLEN) | (1 << FILPBIT_RFLEN)
                                ,
                                (1 << DIRPBIT_ATTR) | (1 << DIRPBIT_ATTR) | (1 << DIRPBIT_FINFO) |
                                (1 << DIRPBIT_CDATE) | (1 << DIRPBIT_BDATE) | (1 << DIRPBIT_MDATE) |
                                (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
                                (1 << DIRPBIT_ACCESS))) {
                clean_exit(ERROR_NETWORK_PROTOCOL);
            }

            if (FPEnumerateFull(Conn, vol, i, 40, DSI_DATASIZ, dir, "",
                                (1 << FILPBIT_LNAME) | (1 << FILPBIT_FNUM) | (1 << FILPBIT_ATTR) |
                                (1 << FILPBIT_FINFO) | (1 << FILPBIT_CDATE) | (1 << FILPBIT_BDATE) |
                                (1 << FILPBIT_MDATE) | (1 << FILPBIT_DFLEN) | (1 << FILPBIT_RFLEN)
                                ,
                                (1 << DIRPBIT_ATTR) | (1 << DIRPBIT_ATTR) | (1 << DIRPBIT_FINFO) |
                                (1 << DIRPBIT_CDATE) | (1 << DIRPBIT_BDATE) | (1 << DIRPBIT_MDATE) |
                                (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
                                (1 << DIRPBIT_ACCESS))) {
                clean_exit(ERROR_NETWORK_PROTOCOL);
            }
        }

        stoptimer();
        addresult(TEST_ENUM2000FILES, iteration_counter);
    }

    /* -------- */
    /* Test (7) */
    if (teststorun[TEST_DELETE2000FILES]) {
#ifdef __linux__
        capture_io_values(TEST_START);
#endif
        starttimer();

        for (i = 1; i <= maxi; i++) {
            snprintf(temp, sizeof(temp), "File.0k%d", i);

            if (FPDelete(Conn, vol, dir, temp)) {
                clean_exit(ERROR_FILE_DIRECTORY_OPS);
            }
        }

        stoptimer();
        addresult(TEST_DELETE2000FILES, iteration_counter);
    }

    /* -------- */
    /* Test (8) */
    if (teststorun[TEST_DIRTREE]) {
        uint32_t idirs[DIRNUM];
        uint32_t jdirs[DIRNUM][DIRNUM];
        uint32_t kdirs[DIRNUM][DIRNUM][DIRNUM];
#ifdef __linux__
        capture_io_values(TEST_START);
#endif
        starttimer();

        for (i = 0; i < DIRNUM; i++) {
            snprintf(temp, sizeof(temp), "dir%02u", i + 1);
            FAILEXIT(!(idirs[i] = FPCreateDir(Conn, vol, dir, temp)), fin1);

            for (j = 0; j < DIRNUM; j++) {
                snprintf(temp, sizeof(temp), "dir%02u", j + 1);
                FAILEXIT(!(jdirs[i][j] = FPCreateDir(Conn, vol, idirs[i], temp)), fin1);

                for (k = 0; k < DIRNUM; k++) {
                    snprintf(temp, sizeof(temp), "dir%02u", k + 1);
                    FAILEXIT(!(kdirs[i][j][k] = FPCreateDir(Conn, vol, jdirs[i][j], temp)), fin1);
                }
            }
        }

        stoptimer();
        addresult(TEST_DIRTREE, iteration_counter);

        for (i = 0; i < DIRNUM; i++) {
            for (j = 0; j < DIRNUM; j++) {
                for (k = 0; k < DIRNUM; k++) {
                    FAILEXIT(FPDelete(Conn, vol, kdirs[i][j][k], "") != 0, fin1);
                }

                FAILEXIT(FPDelete(Conn, vol, jdirs[i][j], "") != 0, fin1);
            }

            FAILEXIT(FPDelete(Conn, vol, idirs[i], "") != 0, fin1);
        }
    }

    /* -------- */
    /* Test (9) - Directory Cache Hits */
    if (teststorun[TEST_CACHE_HITS]) {
        /* Validate configuration to prevent stack overflow */
        if (cache_dirs <= 0 || cache_dirs > 50 || cache_files_per_dir <= 0
                || cache_files_per_dir > 50) {
            fprintf(stderr, "Invalid cache test configuration: dirs=%d, files_per_dir=%d\n",
                    cache_dirs, cache_files_per_dir);
            clean_exit(ERROR_VALIDATION_TEST);
        }

        /* Use dynamic allocation to prevent stack overflow with VLAs */
        char (*cache_test_files)[32] = calloc(cache_dirs * cache_files_per_dir,
                                              sizeof(char[32]));
        int32_t *cache_test_dirs_arr = calloc(cache_dirs, sizeof(int32_t));

        if (!cache_test_files) {
            fprintf(stderr, "Memory allocation failed for test files cache\n");
            clean_exit(ERROR_MEMORY_ALLOCATION);
        }

        if (!cache_test_dirs_arr) {
            fprintf(stderr, "Memory allocation failed for test dirs array cache\n");
            free(cache_test_files);  /* Free the successfully allocated memory from above */
            clean_exit(ERROR_MEMORY_ALLOCATION);
        }

        /* Create test directories */
        for (i = 0; i < cache_dirs; i++) {
            snprintf(temp, sizeof(temp), "cache_dir_%d", i);

            if (!(cache_test_dirs_arr[i] = FPCreateDir(Conn, vol, dir, temp))) {
                free(cache_test_files);
                free(cache_test_dirs_arr);
                clean_exit(ERROR_MEMORY_ALLOCATION);
            }
        }

        /* Create test files in each directory */
        for (i = 0; i < cache_dirs; i++) {
            for (j = 0; j < cache_files_per_dir; j++) {
                snprintf(cache_test_files[i * cache_files_per_dir + j],
                         sizeof(cache_test_files[0]), "cache_file_%d_%d", i, j);

                if (FPCreateFile(Conn, vol, 0, cache_test_dirs_arr[i],
                                 cache_test_files[i * cache_files_per_dir + j])) {
                    free(cache_test_files);
                    free(cache_test_dirs_arr);
                    clean_exit(ERROR_MEMORY_ALLOCATION);
                }
            }
        }

        /* Now perform many repeated lookups to benefit from caching */
#ifdef __linux__
        capture_io_values(TEST_START);
#endif
        starttimer();

        for (k = 0; k < cache_lookup_iterations; k++) {
            for (i = 0; i < cache_dirs; i++) {
                /* Directory lookups - is_there() returns AFP_OK (0) when found */
                snprintf(temp, sizeof(temp), "cache_dir_%d", i);

                if (is_there(Conn, vol, dir, temp) != AFP_OK) {
                    clean_exit(ERROR_FILE_DIRECTORY_OPS);
                }

                /* File parameter requests (should hit cache) */
                for (j = 0; j < cache_files_per_dir; j++) {
                    if (FPGetFileDirParams(Conn, vol, cache_test_dirs_arr[i],
                                           cache_test_files[i * cache_files_per_dir + j],
                                           (1 << FILPBIT_FNUM) | (1 << FILPBIT_PDID) | (1 << FILPBIT_DFLEN), 0)) {
                        clean_exit(ERROR_FILE_DIRECTORY_OPS);
                    }
                }
            }
        }

        stoptimer();
        addresult(TEST_CACHE_HITS, iteration_counter);

        /* Cleanup cache test files and directories */
        for (i = 0; i < cache_dirs; i++) {
            for (j = 0; j < cache_files_per_dir; j++) {
                FPDelete(Conn, vol, cache_test_dirs_arr[i],
                         cache_test_files[i * cache_files_per_dir + j]);
            }

            FPDelete(Conn, vol, cache_test_dirs_arr[i], "");
        }

        free(cache_test_files);
        free(cache_test_dirs_arr);
    }

    /* -------- */
    /* Test (10) - Mixed Cache Operations */
    if (teststorun[TEST_MIXED_CACHE_OPS]) {
#ifdef __linux__
        capture_io_values(TEST_START);
#endif
        starttimer();

        /* Pattern: create -> stat -> enum -> stat -> delete (tests cache lifecycle) */
        for (i = 0; i < mixed_cache_files; i++) {
            snprintf(temp, sizeof(temp), "mixed_file_%d", i);

            /* Create file */
            if (FPCreateFile(Conn, vol, 0, dir, temp)) {
                clean_exit(ERROR_FILE_DIRECTORY_OPS);
            }

            /* Stat it (should cache the entry) */
            if (FPGetFileDirParams(Conn, vol, dir, temp,
                                   (1 << FILPBIT_FNUM) | (1 << FILPBIT_PDID) | (1 << FILPBIT_DFLEN), 0)) {
                clean_exit(ERROR_FILE_DIRECTORY_OPS);
            }

            /* Enumerate directory (should use cached entries) */
            if (i % 10 == 9
                    && FPEnumerate(Conn, vol, dir, "", (1 << FILPBIT_LNAME) | (1 << FILPBIT_FNUM),
                                   0)) {  /* Every 10th iteration */
                /* Enumeration failure is not fatal for this test, just log and continue */
                fprintf(stderr,
                        "Warning: Enumeration failed during mixed cache operations test (iteration %d)\n",
                        i);
            }

            /* Stat again (should hit cache) */
            if (FPGetFileDirParams(Conn, vol, dir, temp,
                                   (1 << FILPBIT_FNUM) | (1 << FILPBIT_CDATE) | (1 << FILPBIT_MDATE), 0)) {
                clean_exit(ERROR_FILE_DIRECTORY_OPS);
            }

            /* Delete (should invalidate cache entry) */
            if (FPDelete(Conn, vol, dir, temp)) {
                clean_exit(ERROR_FILE_DIRECTORY_OPS);
            }
        }

        stoptimer();
        addresult(TEST_MIXED_CACHE_OPS, iteration_counter);
    }

    /* -------- */
    /* Test (11) - Deep Path Traversal */
    if (teststorun[TEST_DEEP_TRAVERSAL]) {
        /* Validate configuration to prevent issues */
        if (deep_dir_levels <= 0 || deep_dir_levels > 100) {
            fprintf(stderr, "Invalid deep directory levels: %d\n", deep_dir_levels);
            clean_exit(ERROR_CONFIGURATION);
        }

        /* Create a deep directory structure */
        uint32_t *deep_dirs = calloc(deep_dir_levels, sizeof(uint32_t));

        if (!deep_dirs) {
            fprintf(stderr, "Memory allocation failed for deep directories\n");
            clean_exit(ERROR_MEMORY_ALLOCATION);
        }

        uint32_t current_dir = dir;

        /* Create deep directory structure */
        for (i = 0; i < deep_dir_levels; i++) {
            snprintf(temp, sizeof(temp), "deep_%02d", i);

            if (!(deep_dirs[i] = FPCreateDir(Conn, vol, current_dir, temp))) {
                free(deep_dirs);
                clean_exit(ERROR_MEMORY_ALLOCATION);
            }

            current_dir = deep_dirs[i];
        }

        /* Create some files in the deepest directory */
        for (i = 0; i < deep_test_files; i++) {
            snprintf(temp, sizeof(temp), "deep_file_%d", i);

            if (FPCreateFile(Conn, vol, 0, current_dir, temp)) {
                free(deep_dirs);
                clean_exit(ERROR_FILE_DIRECTORY_OPS);
            }
        }

#ifdef __linux__
        capture_io_values(TEST_START);
#endif
        starttimer();

        /* Perform deep traversals - this benefits greatly from directory caching */
        for (k = 0; k < deep_traversals; k++) {
            current_dir = dir;

            /* Navigate down the deep structure */
            for (i = 0; i < deep_dir_levels; i++) {
                snprintf(temp, sizeof(temp), "deep_%02d", i);

                /* Directory lookup (should hit cache after first time) - is_there() returns AFP_OK when found */
                if (is_there(Conn, vol, current_dir, temp) != AFP_OK) {
                    free(deep_dirs);
                    clean_exit(ERROR_FILE_DIRECTORY_OPS);
                }

                current_dir = deep_dirs[i];
            }

            for (i = 0; i < deep_test_files; i++) {
                snprintf(temp, sizeof(temp), "deep_file_%d", i);

                if (FPGetFileDirParams(Conn, vol, current_dir, temp,
                                       (1 << FILPBIT_FNUM) | (1 << FILPBIT_DFLEN), 0)) {
                    free(deep_dirs);
                    clean_exit(ERROR_FILE_DIRECTORY_OPS);
                }
            }
        }

        stoptimer();
        addresult(TEST_DEEP_TRAVERSAL, iteration_counter);
        /* Cleanup deep structure - cleanup files from deepest directory first */
        current_dir = deep_dirs[deep_dir_levels - 1];  /* Reset to deepest directory */

        for (i = 0; i < deep_test_files; i++) {
            snprintf(temp, sizeof(temp), "deep_file_%d", i);
            FPDelete(Conn, vol, current_dir, temp);
        }

        /* Delete directories from deepest to shallowest */
        for (i = deep_dir_levels - 1; i >= 0; i--) {
            FPDelete(Conn, vol, deep_dirs[i], "");
        }

        /* Free dynamically allocated memory */
        free(deep_dirs);
    }

    /* -------- */
    /* Test (12) - Cache Validation Efficiency */
    if (teststorun[TEST_CACHE_VALIDATION]) {
        /* Validate configuration parameters */
        if (validation_files <= 0 || validation_files > 1000 ||
                validation_iterations <= 0 || validation_iterations > 1000) {
            fprintf(stderr,
                    "Invalid validation test configuration: files=%d, iterations=%d\n",
                    validation_files, validation_iterations);
            clean_exit(ERROR_VALIDATION_TEST);
        }

        /* Create test files for validation testing */
        for (i = 0; i < validation_files; i++) {
            snprintf(temp, sizeof(temp), "valid_file_%d", i);

            if (FPCreateFile(Conn, vol, 0, dir, temp)) {
                clean_exit(ERROR_FILE_DIRECTORY_OPS);
            }
        }

#ifdef __linux__
        capture_io_values(TEST_START);
#endif
        starttimer();

        /* Perform many repeated access operations
         * With the improved cache validation, most of these should skip
         * the expensive filesystem validation calls */
        for (k = 0; k < validation_iterations; k++) {
            for (i = 0; i < validation_files; i++) {
                snprintf(temp, sizeof(temp), "valid_file_%d", i);

                /* Multiple parameter requests on same file */
                if (FPGetFileDirParams(Conn, vol, dir, temp,
                                       (1 << FILPBIT_FNUM) | (1 << FILPBIT_PDID), 0)) {
                    clean_exit(ERROR_FILE_DIRECTORY_OPS);
                }

                if (FPGetFileDirParams(Conn, vol, dir, temp,
                                       (1 << FILPBIT_DFLEN) | (1 << FILPBIT_CDATE), 0)) {
                    clean_exit(ERROR_FILE_DIRECTORY_OPS);
                }

                if (FPGetFileDirParams(Conn, vol, dir, temp,
                                       (1 << FILPBIT_MDATE) | (1 << FILPBIT_FINFO), 0)) {
                    clean_exit(ERROR_FILE_DIRECTORY_OPS);
                }
            }
        }

        stoptimer();
        addresult(TEST_CACHE_VALIDATION, iteration_counter);

        /* Cleanup validation test files */
        for (i = 0; i < validation_files; i++) {
            snprintf(temp, sizeof(temp), "valid_file_%d", i);
            FPDelete(Conn, vol, dir, temp);
        }
    }

fin1:
fin:
    return;
}

/* =============================== */
void usage(char *av0)
{
    int32_t i = 0;
    fprintf(stdout,
            "usage:\t%s [-34567bcGgKVv] [-h host] [-p port] [-s vol] [-u user] [-w password] "
            "[-n iterations] [-f tests] [-F bigfile]\n", av0);
    fprintf(stdout, "\t-h\tserver host name (default localhost)\n");
    fprintf(stdout, "\t-p\tserver port (default 548)\n");
    fprintf(stdout, "\t-s\tvolume to mount\n");
    fprintf(stdout, "\t-u\tuser name (default uid)\n");
    fprintf(stdout, "\t-w\tpassword\n");
    fprintf(stdout, "\t-3\tAFP 3.0 version\n");
    fprintf(stdout, "\t-4\tAFP 3.1 version\n");
    fprintf(stdout, "\t-5\tAFP 3.2 version\n");
    fprintf(stdout, "\t-6\tAFP 3.3 version\n");
    fprintf(stdout, "\t-7\tAFP 3.4 version (default)\n");
    fprintf(stdout, "\t-b\tdebug mode\n");
    fprintf(stdout, "\t-c\toutput results in CSV format (default: tabular)\n");
    fprintf(stdout, "\t-K\trun cache-focused tests only (tests 8-11)\n");
    fprintf(stdout, "\t-f\ttests to run, eg 134 for tests 1, 3 and 4\n");
    fprintf(stdout,
            "\t-F\tuse this file located in the volume root for the read test (size must match -g and -G options)\n");
    fprintf(stdout, "\t-g\tfast network (Gbit, file testsize 1 GB)\n");
    fprintf(stdout,
            "\t-G\tridiculously fast network (10 Gbit, file testsize 10 GB)\n");
    fprintf(stdout,
            "\t-n\titerations to run (default: 2), for iterations > 5 outliers will be removed\n");
    fprintf(stdout, "\t-v\tverbose\n");
    fprintf(stdout, "\t-V\tvery verbose\n");
    fprintf(stdout, "\tAvailable tests:\n");

    for (i = 0; i < NUMTESTS; i++) {
        fprintf(stdout, "\t(%u) %s", i + 1, test_names[i]);

        if (i >= TEST_CACHE_HITS) {
            fprintf(stdout, " [CACHE]");
        }

        fprintf(stdout, "\n");
    }

    fprintf(stdout, "\n\tCache-focused tests (8-11) are designed to highlight\n");
    fprintf(stdout, "\tdirectory cache performance improvements in netatalk.\n");
    fprintf(stdout, "\tThese tests benefit significantly from optimized cache\n");
    fprintf(stdout, "\tvalidation and probabilistic validation features.\n");
    exit(1);
}

/* ------------------------------- */
int main(int32_t ac, char **av)
{
    int32_t cc, i, t;
    static char *vers = "AFP3.4";
    static char *uam = "Cleartxt Passwrd";

    if (ac == 1) {
        usage(av[0]);
    }

    while ((cc = getopt(ac, av, "34567bcGgKVvF:f:h:n:p:s:u:w:")) != EOF) {
        switch (cc) {
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

        case 'b':
            Debug = 1;
            break;

        case 'K':
            /* Special handling for cache tests - set them directly */
            memset(teststorun, 0, NUMTESTS);
            teststorun[TEST_CACHE_HITS] = 1;
            teststorun[TEST_MIXED_CACHE_OPS] = 1;
            teststorun[TEST_DEEP_TRAVERSAL] = 1;
            teststorun[TEST_CACHE_VALIDATION] = 1;
            Test = strdup("K");  /* Mark as cache-only mode */

            if (!Test) {
                fprintf(stderr, "Memory allocation failed for Test string\n");
                exit(1);
            }

            break;

        case 'c':
            csv_output = 1;
            break;

        case 'F':
            bigfilename = strdup(optarg);

            if (!bigfilename) {
                fprintf(stderr, "Memory allocation failed for big filename\n");
                exit(1);
            }

            break;

        case 'f':
            Test = strdup(optarg);

            if (!Test) {
                fprintf(stderr, "Memory allocation failed for Test string\n");
                exit(1);
            }

            break;

        case 'G':
            rwsize *= 100;
            break;

        case 'g':
            rwsize *= 10;
            break;

        case 'h':
            Server = strdup(optarg);

            if (!Server) {
                fprintf(stderr, "Memory allocation failed for Server string\n");
                exit(1);
            }

            break;

        case 'n':
            Iterations = safe_atoi(optarg, "iterations", 1, 256);

            if (Iterations <= 0) {
                fprintf(stderr, "Error: Number of iterations must be positive (got %d)\n",
                        Iterations);
                exit(1);
            }

            break;

        case 'p' :
            Port = safe_atoi(optarg, "port", 1, 65535);

            if (Port <= 0) {
                fprintf(stderr, "Bad port.\n");
                exit(1);
            }

            break;

        case 's':
            Vol = strdup(optarg);

            if (!Vol) {
                fprintf(stderr, "Memory allocation failed for Vol string\n");
                exit(1);
            }

            break;

        case 'u':
            User = strdup(optarg);

            if (!User) {
                fprintf(stderr, "Memory allocation failed for User string\n");
                exit(1);
            }

            break;

        case 'V':
            lantest_Quiet = 0;
            Verbose = 1;
            break;

        case 'v':
            lantest_Quiet = 0;
            break;

        case 'w':
            Password = strdup(optarg);

            if (!Password) {
                fprintf(stderr, "Memory allocation failed for Password string\n");
                exit(1);
            }

            break;

        default :
            usage(av[0]);
        }
    }

    if (User[0] == '\0') {
        struct passwd pwd;
        struct passwd *result = NULL;
        uid_t uid = geteuid();
        char buf[1024];
        int32_t ret;
        ret = getpwuid_r(uid, &pwd, buf, sizeof(buf), &result);

        if (ret == 0 && result != NULL) {
            User = pwd.pw_name;
            printf("Using current user: %s\n", User);
        }
    }

    if (!lantest_Quiet) {
        fprintf(stdout, "Connecting to host %s:%d\n", Server, Port);
    }

    if (User != NULL && User[0] == '\0') {
        fprintf(stderr, "Error: Define a user with -u\n");
        exit(1);
    }

    if (Password != NULL && Password[0] == '\0') {
        fprintf(stderr, "Error: Define a password with -w\n");
        exit(1);
    }

    if (Vol != NULL && Vol[0] == '\0') {
        fprintf(stderr, "Error: Define a volume with -s\n");
        exit(1);
    }

    if (lantest_Quiet && freopen("/dev/null", "w", stdout) == NULL) {
        fprintf(stderr, "Error: Could not redirect stdout to /dev/null\n");
        exit(1);
    }

#if 0

    /* FIXME: guest auth leads to broken DSI request */
    if ((User[0] == 0) || (Password[0] == 0)) {
        uam = "No User Authent";
    }

#endif
    Iterations_save = Iterations;
    /* Allocate memory for 3D results array: Iterations x NUMTESTS x NUM_MEASUREMENTS */
    results = calloc(Iterations, sizeof(uint64_t[NUMTESTS][NUM_MEASUREMENTS]));

    if (!results) {
        fprintf(stderr,
                "Memory allocation failed for 3D results array (%d iterations)\n",
                Iterations);
        clean_exit(ERROR_VALIDATION_TEST);
    }

    if (!Test) {
        memset(teststorun, 1, NUMTESTS);
    } else if (strcmp(Test, "K") == 0) {
        /* Cache-only mode was already set when parsing -K option */
    } else {
        i = 0;

        for (; Test[i]; i++) {
            t = Test[i] - '1';

            if ((t >= 0) && (t < NUMTESTS)) {
                teststorun[t] = 1;
            }
        }

        if (teststorun[TEST_READ100MB] && !bigfilename) {
            teststorun[TEST_WRITE100MB] = 1;
        }

        if (teststorun[TEST_ENUM2000FILES]) {
            teststorun[TEST_CREATE2000FILES] = 1;
        }

        if (teststorun[TEST_DELETE2000FILES]) {
            teststorun[TEST_CREATE2000FILES] = 1;
        }

        if (Test) {
            free(Test);
        }
    }

    /* Setup Connection structure */
    if ((Conn = (CONN *)calloc(1, sizeof(CONN))) == NULL) {
        fprintf(stderr, "Memory allocation failed for connection structure\n");
        return 1;
    }

    /* Open socket and setup DSI connection */
    int32_t sock;
    dsi = &Conn->dsi;
    sock = OpenClientSocket(Server, Port);

    if (sock < 0) {
        return 2;
    }

    dsi->socket = sock;
    Conn->afp_version = Version;

    /* Login to AFP server and open Vol */
    if (Version >= 30) {
        ExitCode = ntohs(FPopenLoginExt(Conn, vers, uam, User, Password));
    } else {
        ExitCode = ntohs(FPopenLogin(Conn, vers, uam, User, Password));
    }

    vol  = FPOpenVol(Conn, Vol);

    if (vol == 0xffff) {
        fprintf(stderr, "Error: Failed to open volume '%s'\n", Vol);
        clean_exit(ERROR_VOLUME_OPERATIONS);
    }

#ifdef __linux__
    /* IO monitoring setup - only after AFP connection is established */
    init_io_monitoring(User);

    /* Warn if not localhost - only warn to allow testing with local interface IP for TCP network stack tests */
    if (strcmp(Server, "localhost") != 0 && strcmp(Server, "127.0.0.1") != 0
            && strstr(Server, "::") != NULL) {
        fprintf(stderr,
                "Warning: IO monitoring only works when afp_lantest is executed on the same host as the Netatalk server.\n"
                "         Current server: %s (not localhost or 127.0.0.1)\n", Server);
    }

    fprintf(stdout, "\n");
#endif /* __linux__ */
    int32_t dir;
    char testdir[MAXPATHLEN];
    snprintf(testdir, sizeof(testdir), "LanTest-%d", getpid());

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "", 0, (1 << DIRPBIT_DID))) {
        clean_exit(ERROR_FILE_DIRECTORY_OPS);
    }

    /* Delete any existing testdir directory first to avoid conflicts */
    if (is_there(Conn, vol, DIRDID_ROOT, testdir) == AFP_OK) {
        if (!lantest_Quiet) {
            fprintf(stdout, "Found existing test directory '%s', deleting it...\n",
                    testdir);
        }

        if (FPDelete(Conn, vol, DIRDID_ROOT, testdir)) {
            fprintf(stderr, "ERROR: Could not delete existing test directory '%s'\n",
                    testdir);
            fprintf(stderr,
                    "The directory may not be empty. Please manually delete it and try again.\n");
            clean_exit(ERROR_FILE_DIRECTORY_OPS);
        }

        /* Verify the directory was actually deleted */
        if (is_there(Conn, vol, DIRDID_ROOT, testdir) == AFP_OK) {
            fprintf(stderr,
                    "ERROR: Test directory '%s' still exists after deletion attempt\n", testdir);
            clean_exit(ERROR_FILE_DIRECTORY_OPS);
        }
    }

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, testdir))) {
        clean_exit(ERROR_FILE_DIRECTORY_OPS);
    }

    if (FPGetFileDirParams(Conn, vol, dir, "", 0, (1 << DIRPBIT_DID))) {
        clean_exit(ERROR_FILE_DIRECTORY_OPS);
    }

    if (is_there(Conn, vol, DIRDID_ROOT, testdir) != AFP_OK) {
        clean_exit(ERROR_FILE_DIRECTORY_OPS);
    }

    for (int32_t current_iteration = 0; current_iteration < Iterations;
            current_iteration++) {
        /* Update global iteration counter for addresult() calls */
        iteration_counter = current_iteration;
        run_test(dir);
    }

    FPDelete(Conn, vol, dir, "");

    if (ExitCode != AFP_OK && !Debug) {
        printf("Error, ExitCode: %u. Run with -v or -b to see what went wrong.\n",
               ExitCode);
        clean_exit(ExitCode);
    }

    displayresults();
    clean_exit(ExitCode);
}
