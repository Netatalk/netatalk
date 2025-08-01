#include "specs.h"

#include <getopt.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <pwd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <math.h>
#include <stdint.h>

#include "afpclient.h"
#include "test.h"

#define FPWRITE_RPLY_SIZE 24
#define FPWRITE_RQST_SIZE 36

#define TEST_OPENSTATREAD 0
#define TEST_WRITE100MB 1
#define TEST_READ100MB 2
#define TEST_LOCKUNLOCK 3
#define TEST_CREATE2000FILES 4
#define TEST_ENUM2000FILES 5
#define TEST_DIRTREE 6
#define TEST_CACHE_HITS 7
#define TEST_MIXED_CACHE_OPS 8
#define TEST_DEEP_TRAVERSAL 9
#define TEST_CACHE_VALIDATION 10
#define LASTTEST TEST_CACHE_VALIDATION
#define NUMTESTS (LASTTEST+1)

CONN *Conn;
int ExitCode = 0;
char Data[300000] = "";
char    *Vol = "";
char    *User = "";
char    *Path;
int     Version = 34;
int     Mac = 0;
char    *Test = NULL;

/* Unused */
CONN *Conn2;
uint16_t VolID;
int PassCount = 0;
int FailCount = 0;
int SkipCount = 0;
int NotTestedCount = 0;
char FailedTests[1024][256] = {{0}};
char NotTestedTests[1024][256] = {{0}};
char SkippedTests[1024][256] = {{0}};

/* Configure the tests */
#define DIRNUM 10                                 /* 10^3 nested dirs */
static int smallfiles = 1000;                     /* 1000 files */
static off_t rwsize = 100 * 1024 * 1024;          /* 100 MB */
static int locking = 10000 / 40;                  /* 10000 times */
static int create_enum_files = 2000;              /* 2000 files */

/* Cache test configuration */
static int cache_dirs = 10;                /* dirs for cache tests */
static int cache_files_per_dir = 10;       /* files per dir */
static int cache_lookup_iterations = 100;  /* cache test iterations */
static int mixed_cache_files = 200;        /* mixed ops file count */
static int deep_dir_levels = 20;           /* deep traversal dir levels */
static int deep_test_files = 50;           /* files in deepest dir */
static int deep_traversals = 50;           /* path traversal iterations */
static int validation_files = 100;         /* validation file count */
static int validation_iterations = 100;    /* validation iterations */

static uint16_t vol;
static DSI *dsi;
static char    *Server = "localhost";
static int     Port = DSI_AFPOVERTCP_PORT;
static char    *Password = "";
static int     Iterations = 1;
static int     Iterations_save;
static int     iteration_counter = 0;
static struct timeval tv_start;
static struct timeval tv_end;
static struct timeval tv_dif;
static pthread_t tid;
static unsigned long long numrw;
static char teststorun[NUMTESTS];
static unsigned long (*results)[][NUMTESTS];
static char *bigfilename;

static char *resultstrings[] = {
    "Opening, stating and reading 512 bytes from 1000 files ",
    "Writing one large file                                 ",
    "Reading one large file                                 ",
    "Locking/Unlocking 10000 times each                     ",
    "Creating dir with 2000 files                           ",
    "Enumerate dir with 2000 files                          ",
    "Create directory tree with 10^3 dirs                   ",
    "Directory cache hits (1000 dir + 10000 file lookups)   ",
    "Mixed cache operations (create/stat/enum/delete)       ",
    "Deep path traversal (nested directory navigation)      ",
    "Cache validation efficiency (metadata changes)         "
};

static void starttimer(void)
{
    gettimeofday(&tv_start, NULL);
}

static void stoptimer(void)
{
    gettimeofday(&tv_end, NULL);
}

static unsigned long timediff(void)
{
    if (tv_end.tv_usec < tv_start.tv_usec) {
        tv_end.tv_usec += 1000000;
        tv_end.tv_sec -= 1;
    }

    tv_dif.tv_sec = tv_end.tv_sec - tv_start.tv_sec;
    tv_dif.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
    return (tv_dif.tv_sec * 1000) + (tv_dif.tv_usec / 1000);
}

static void addresult(int test, int iteration)
{
    unsigned long t;
    unsigned long long avg;
    t = timediff();
    fprintf(stderr, "Run %u => %s%6lu ms", iteration, resultstrings[test], t);

    if ((test == TEST_WRITE100MB) || (test == TEST_READ100MB)) {
        if (t > 0) {  /* Prevent division by zero */
            avg = (rwsize / 1000) / t;
            fprintf(stderr, " for %lu MB (avg. %llu MB/s)", rwsize / (1024 * 1024), avg);
        } else {
            fprintf(stderr, " for %lu MB (time too small to measure)",
                    rwsize / (1024 * 1024));
        }
    }

    fprintf(stderr, "\n");
    (*results)[iteration][test] = t;
}

static void displayresults(void)
{
    int i, test, maxindex, minindex, divsub = 0;
    unsigned long long sum, max, min;

    /* Eleminate runaways */
    if (Iterations_save > 5) {
        divsub = 2;

        for (test = 0; test != NUMTESTS; test++) {
            if (! teststorun[test]) {
                continue;
            }

            /* Reset min/max for each test */
            max = 0;
            min = UINT64_MAX;
            maxindex = 0;
            minindex = 0;

            for (i = 0; i < Iterations_save; i++) {
                if ((*results)[i][test] < min) {
                    min = (*results)[i][test];
                    minindex = i;
                }

                if ((*results)[i][test] > max) {
                    max = (*results)[i][test];
                    maxindex = i;
                }
            }

            (*results)[minindex][test] = 0;
            (*results)[maxindex][test] = 0;
        }
    }

    fprintf(stderr, "\nNetatalk Lantest Results");

    if (Iterations_save > 1) {
        fprintf(stderr, " (average times across %d iterations)", Iterations_save);
    } else {
        fprintf(stderr, " (single iteration)");
    }

    fprintf(stderr, "\n");
    fprintf(stderr, "===================================\n\n");
    unsigned long long avg, thrput;
    double std_dev;

    for (test = 0; test < NUMTESTS; test++) {
        if (! teststorun[test]) {
            continue;
        }

        /* Calculate sum and count valid values */
        int valid_count = 0;

        for (i = 0, sum = 0; i < Iterations_save; i++) {
            /* Skip outliers that were set to 0 during elimination */
            if ((*results)[i][test] > 0) {
                sum += (*results)[i][test];
                valid_count++;
            }
        }

        if (valid_count <= 0) {
            fprintf(stderr, "\tFATAL ERROR: no valid iterations for averaging\n");
            exit(1);
        }

        avg = sum / valid_count;

        if (avg < 1) {
            fprintf(stderr, "\tFATAL ERROR: invalid result\n");
            exit(1);
        }

        /* Calculate standard deviation if we have multiple valid iterations */
        std_dev = 0.0;

        if (valid_count > 1) {
            double sum_sq_diff = 0.0;

            for (i = 0; i < Iterations_save; i++) {
                if ((*results)[i][test] > 0) {
                    double diff = (double)((*results)[i][test]) - (double)avg;
                    sum_sq_diff += diff * diff;
                }
            }

            std_dev = sqrt(sum_sq_diff / (valid_count - 1));
        }

        /* Display test result */
        fprintf(stderr, "Test %d: %s", test + 1, resultstrings[test]);

        if (Iterations_save > 1) {
            fprintf(stderr, "\n\t  Average: %6llu ms", avg);

            if (valid_count > 1) {
                fprintf(stderr, " ± %.1f ms (std dev)", std_dev);
            }
        } else {
            fprintf(stderr, ": %6llu ms", avg);
        }

        if (test == TEST_WRITE100MB) {
            thrput = (rwsize / 1000) / avg;
            fprintf(stderr, "\n\t  Throughput: %llu MB/s (Write, %lu MB file)", thrput,
                    rwsize / (1024 * 1024));
        }

        if (test == TEST_READ100MB) {
            thrput = (rwsize / 1000) / avg;
            fprintf(stderr, "\n\t  Throughput: %llu MB/s (Read, %lu MB file)", thrput,
                    rwsize / (1024 * 1024));
        }

        fprintf(stderr, "\n");

        if (Iterations_save > 1) {
            fprintf(stderr, "\n");
        }
    }
}

/* ------------------------- */
void fatal_failed(void)
{
    fprintf(stderr, "\tFATAL ERROR\n");
    exit(1);
}

/* --------------------------------- */
int is_there(CONN *conn, int did, char *name)
{
    return FPGetFileDirParams(conn, vol, did, name,
                              (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID)
                              ,
                              (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID)
                             );
}

struct async_io_req {
    unsigned long long air_count;
    size_t air_size;
};

static void *rply_thread(void *p)
{
    struct async_io_req *air = p;
    size_t size = air->air_size;
    unsigned long long n = air->air_count;
    size_t stored;
    ssize_t len;
    char *buf = NULL;
    buf = malloc(size);

    if (!buf) {
        fprintf(stderr, "Memory allocation failed in rply_thread\n");
        return NULL;
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
void run_test(const int dir)
{
    static char *data;
    int i, maxi = 0;
    int j, k;
    int fork;
    int test = 0;
    int nowrite;
    off_t offset;
    struct async_io_req air;
    static char temp[MAXPATHLEN];
    numrw = rwsize / (dsi->server_quantum - FPWRITE_RQST_SIZE);

    if (!data) {
        data = calloc(1, dsi->server_quantum);

        if (!data) {
            fprintf(stderr, "Memory allocation failed for data buffer\n");
            fatal_failed();
        }
    }

    /* --------------- */
    /* Test (1)        */
    if (teststorun[TEST_OPENSTATREAD]) {
        for (i = 0; i < smallfiles; i++) {
            snprintf(temp, sizeof(temp), "File.small%d", i);

            if (ntohl(AFPERR_NOOBJ) != is_there(Conn, dir, temp)) {
                fatal_failed();
            }

            if (FPGetFileDirParams(Conn, vol, dir, "", 0, (1 << DIRPBIT_DID))) {
                fatal_failed();
            }

            if (FPCreateFile(Conn, vol, 0, dir, temp)) {
                fatal_failed();
            }

            if (is_there(Conn, dir, temp)) {
                fatal_failed();
            }

            if (FPGetFileDirParams(Conn, vol, dir, temp,
                                   (1 << FILPBIT_FNUM) | (1 << FILPBIT_PDID) | (1 << FILPBIT_FINFO) |
                                   (1 << FILPBIT_CDATE) | (1 << FILPBIT_MDATE) | (1 << FILPBIT_DFLEN) |
                                   (1 << FILPBIT_RFLEN)
                                   , 0)) {
                fatal_failed();
            }

            if (FPGetFileDirParams(Conn, vol, dir, temp,
                                   (1 << FILPBIT_FNUM) | (1 << FILPBIT_PDID) | (1 << FILPBIT_FINFO) |
                                   (1 << DIRPBIT_ATTR) | (1 << DIRPBIT_BDATE) |
                                   (1 << FILPBIT_CDATE) | (1 << FILPBIT_MDATE) | (1 << FILPBIT_DFLEN) |
                                   (1 << FILPBIT_RFLEN)
                                   , 0)) {
                fatal_failed();
            }

            fork = FPOpenFork(Conn, vol, OPENFORK_DATA,
                              (1 << FILPBIT_PDID) | (1 << DIRPBIT_LNAME) | (1 << FILPBIT_FNUM) |
                              (1 << FILPBIT_DFLEN)
                              , dir, temp, OPENACC_WR | OPENACC_RD | OPENACC_DWR | OPENACC_DRD);

            if (!fork) {
                fatal_failed();
            }

            if (FPGetForkParam(Conn, fork,
                               (1 << FILPBIT_PDID) | (1 << DIRPBIT_LNAME) | (1 << FILPBIT_DFLEN))) {
                fatal_failed();
            }

            if (FPWrite(Conn, fork, 0, 20480, data, 0)) {
                fatal_failed();
            }

            if (FPCloseFork(Conn, fork)) {
                fatal_failed();
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
            fatal_failed();
        }

        starttimer();

        for (i = 0; i <= maxi; i++) {
            snprintf(temp, sizeof(temp), "File.small%d", i);

            if (is_there(Conn, dir, temp)) {
                fatal_failed();
            }

            if (FPGetFileDirParams(Conn, vol, dir, temp, 0x72d, 0)) {
                fatal_failed();
            }

            if (FPGetFileDirParams(Conn, vol, dir, temp, 0x73f, 0x133f)) {
                fatal_failed();
            }

            fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0x342, dir, temp, OPENACC_RD);

            if (!fork) {
                fatal_failed();
            }

            if (FPGetForkParam(Conn, fork, (1 << FILPBIT_DFLEN))) {
                fatal_failed();
            }

            if (FPRead(Conn, fork, 0, 512, data)) {
                fatal_failed();
            }

            if (FPCloseFork(Conn, fork)) {
                fatal_failed();
            }

            fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0x342, dir, temp,
                              OPENACC_RD | OPENACC_DWR);

            if (!fork) {
                fatal_failed();
            }

            if (FPGetForkParam(Conn, fork, 0x242)) {
                fatal_failed();
            }

            if (FPGetFileDirParams(Conn, vol, dir, temp, 0x72d, 0)) {
                fatal_failed();
            }

            if (FPCloseFork(Conn, fork)) {
                fatal_failed();
            }
        }

        stoptimer();
        addresult(TEST_OPENSTATREAD, iteration_counter);

        /* ---------------- */
        for (i = 0; i <= maxi; i++) {
            snprintf(temp, sizeof(temp), "File.small%d", i);

            if (is_there(Conn, dir, temp)) {
                fatal_failed();
            }

            if (FPGetFileDirParams(Conn, vol, dir, temp, 0, (1 << FILPBIT_FNUM))) {
                fatal_failed();
            }

            if (FPDelete(Conn, vol, dir, temp)) {
                fatal_failed();
            }
        }

        if (FPGetVolParam(Conn, vol, (1 << VOLPBIT_MDATE) | (1 << VOLPBIT_XBFREE))) {
            fatal_failed();
        }
    }

    /* -------- */
    /* Test (2) */
    if (teststorun[TEST_WRITE100MB]) {
        strcpy(temp, "File.big");

        if (FPCreateFile(Conn, vol, 0, dir, temp)) {
            fatal_failed();
        }

        if (FPGetFileDirParams(Conn, vol, dir, temp, 0x72d, 0)) {
            fatal_failed();
        }

        if (FPGetFileDirParams(Conn, vol, dir, temp, 0x73f, 0x133f)) {
            fatal_failed();
        }

        fork = FPOpenFork(Conn, vol, OPENFORK_DATA,
                          (1 << FILPBIT_PDID) | (1 << FILPBIT_FNUM) | (1 << FILPBIT_DFLEN)
                          , dir, temp, OPENACC_WR | OPENACC_RD | OPENACC_DWR | OPENACC_DRD);

        if (!fork) {
            fatal_failed();
        }

        if (FPGetForkParam(Conn, fork, (1 << FILPBIT_PDID))) {
            fatal_failed();
        }

        air.air_count = numrw;
        air.air_size = FPWRITE_RPLY_SIZE;
        int pthread_ret = pthread_create(&tid, NULL, rply_thread, &air);

        if (pthread_ret != 0) {
            fprintf(stderr, "pthread_create failed: %d\n", pthread_ret);
            fatal_failed();
        }

        starttimer();

        for (i = 0, offset = 0; i < numrw;
                offset += (dsi->server_quantum - FPWRITE_RQST_SIZE), i++) {
            if (FPWrite_ext_async(Conn, fork, offset,
                                  dsi->server_quantum - FPWRITE_RQST_SIZE, data, 0)) {
                fatal_failed();
            }
        }

        pthread_ret = pthread_join(tid, NULL);

        if (pthread_ret != 0) {
            fprintf(stderr, "pthread_join failed: %d\n", pthread_ret);
            fatal_failed();
        }

        if (FPCloseFork(Conn, fork)) {
            fatal_failed();
        }

        stoptimer();
        addresult(TEST_WRITE100MB, iteration_counter);
    }

    /* -------- */
    /* Test (3) */
    if (teststorun[TEST_READ100MB]) {
        if (!bigfilename) {
            if (is_there(Conn, dir, temp) != AFP_OK) {
                fatal_failed();
            }

            fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0x342, dir, temp,
                              OPENACC_RD | OPENACC_DWR);
        } else {
            if (is_there(Conn, DIRDID_ROOT, bigfilename) != AFP_OK) {
                fatal_failed();
            }

            fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0x342, DIRDID_ROOT, bigfilename,
                              OPENACC_RD | OPENACC_DWR);
        }

        if (!fork) {
            fatal_failed();
        }

        if (FPGetForkParam(Conn, fork, 0x242)) {
            fatal_failed();
        }

        air.air_count = numrw;
        air.air_size = (dsi->server_quantum - FPWRITE_RQST_SIZE) + 16;
        int pthread_ret = pthread_create(&tid, NULL, rply_thread, &air);

        if (pthread_ret != 0) {
            fprintf(stderr, "pthread_create failed: %d\n", pthread_ret);
            fatal_failed();
        }

        starttimer();

        for (i = 0; i < numrw ; i++) {
            if (FPRead_ext_async(Conn, fork, i * (dsi->server_quantum - FPWRITE_RQST_SIZE),
                                 dsi->server_quantum - FPWRITE_RQST_SIZE, data)) {
                fatal_failed();
            }
        }

        pthread_ret = pthread_join(tid, NULL);

        if (pthread_ret != 0) {
            fprintf(stderr, "pthread_join failed: %d\n", pthread_ret);
            fatal_failed();
        }

        if (FPCloseFork(Conn, fork)) {
            fatal_failed();
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
        strcpy(temp, "File.lock");

        if (ntohl(AFPERR_NOOBJ) != is_there(Conn, dir, temp)) {
            fatal_failed();
        }

        if (FPGetFileDirParams(Conn, vol, dir, "", 0, (1 << DIRPBIT_DID))) {
            fatal_failed();
        }

        if (FPCreateFile(Conn, vol, 0, dir, temp)) {
            fatal_failed();
        }

        if (is_there(Conn, dir, temp)) {
            fatal_failed();
        }

        if (FPGetFileDirParams(Conn, vol, dir, temp,
                               (1 << FILPBIT_FNUM) | (1 << FILPBIT_PDID) | (1 << FILPBIT_FINFO) |
                               (1 << FILPBIT_CDATE) | (1 << FILPBIT_DFLEN) | (1 << FILPBIT_RFLEN)
                               , 0)) {
            fatal_failed();
        }

        fork = FPOpenFork(Conn, vol, OPENFORK_DATA,
                          (1 << FILPBIT_PDID) | (1 << FILPBIT_FNUM) | (1 << FILPBIT_DFLEN)
                          , dir, temp, OPENACC_WR | OPENACC_RD | OPENACC_DWR | OPENACC_DRD);

        if (!fork) {
            fatal_failed();
        }

        if (FPGetForkParam(Conn, fork, (1 << FILPBIT_PDID) | (1 << FILPBIT_DFLEN))) {
            fatal_failed();
        }

        if (FPGetFileDirParams(Conn, vol, dir, temp,
                               (1 << DIRPBIT_ATTR) | (1 << FILPBIT_CDATE) | (1 << FILPBIT_MDATE) |
                               (1 << FILPBIT_FNUM) |
                               (1 << FILPBIT_FINFO) | (1 << FILPBIT_DFLEN) | (1 << FILPBIT_RFLEN)
                               , 0)) {
            fatal_failed();
        }

        if (FPWrite(Conn, fork, 0, 40000, data, 0)) {
            fatal_failed();
        }

        if (FPCloseFork(Conn, fork)) {
            fatal_failed();
        }

        if (is_there(Conn, dir, temp)) {
            fatal_failed();
        }

        if (FPGetFileDirParams(Conn, vol, dir, temp, 0x73f, 0x133f)) {
            fatal_failed();
        }

        fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0x342, dir, temp, OPENACC_RD);

        if (!fork) {
            fatal_failed();
        }

        if (FPGetForkParam(Conn, fork, (1 << FILPBIT_DFLEN))) {
            fatal_failed();
        }

        if (FPRead(Conn, fork, 0, 512, data)) {
            fatal_failed();
        }

        if (FPCloseFork(Conn, fork)) {
            fatal_failed();
        }

        fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0x342, dir, temp,
                          OPENACC_RD | OPENACC_WR);

        if (!fork) {
            fatal_failed();
        }

        if (FPGetForkParam(Conn, fork, 0x242)) {
            fatal_failed();
        }

        if (FPGetFileDirParams(Conn, vol, dir, temp, 0x72d, 0)) {
            fatal_failed();
        }

        starttimer();

        for (j = 0; j < locking; j++) {
            for (i = 0; i <= 390; i += 10) {
                if (FPByteLock(Conn, fork, 0, 0, i, 10)) {
                    fatal_failed();
                }

                if (FPByteLock(Conn, fork, 0, 1, i, 10)) {
                    fatal_failed();
                }
            }
        }

        stoptimer();
        addresult(TEST_LOCKUNLOCK, iteration_counter);

        if (is_there(Conn, dir, temp)) {
            fatal_failed();
        }

        if (FPCloseFork(Conn, fork)) {
            fatal_failed();
        }

        if (FPDelete(Conn, vol, dir, "File.lock")) {
            fatal_failed();
        }
    }

    /* -------- */
    /* Test (5) */
    if (teststorun[TEST_CREATE2000FILES]) {
        starttimer();

        for (i = 1; i <= create_enum_files; i++) {
            snprintf(temp, sizeof(temp), "File.0k%d", i);

            if (FPCreateFile(Conn, vol, 0, dir, temp)) {
                fatal_failed();
                break;
            }

            if (FPGetFileDirParams(Conn, vol, dir, temp,
                                   (1 << FILPBIT_FNUM) | (1 << FILPBIT_PDID) | (1 << FILPBIT_FINFO) |
                                   (1 << FILPBIT_CDATE) | (1 << FILPBIT_DFLEN) | (1 << FILPBIT_RFLEN)
                                   , 0) != AFP_OK) {
                fatal_failed();
            }

            maxi = i;  /* Track last successful file creation */
        }

        stoptimer();
        addresult(TEST_CREATE2000FILES, iteration_counter);
    }

    /* -------- */
    /* Test (6) */
    if (teststorun[TEST_ENUM2000FILES]) {
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
            fatal_failed();
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
                fatal_failed();
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
                fatal_failed();
            }
        }

        stoptimer();
        addresult(TEST_ENUM2000FILES, iteration_counter);
    }

    /* Delete files from Test (5/6) */
    if (teststorun[TEST_CREATE2000FILES]) {
        for (i = 1; i <= maxi; i++) {
            snprintf(temp, sizeof(temp), "File.0k%d", i);

            if (FPDelete(Conn, vol, dir, temp)) {
                fatal_failed();
            }
        }
    }

    /* -------- */
    /* Test (7) */
    if (teststorun[TEST_DIRTREE]) {
        uint32_t idirs[DIRNUM];
        uint32_t jdirs[DIRNUM][DIRNUM];
        uint32_t kdirs[DIRNUM][DIRNUM][DIRNUM];
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
    /* Test (8) - Directory Cache Hits */
    if (teststorun[TEST_CACHE_HITS]) {
        /* Validate configuration to prevent stack overflow */
        if (cache_dirs <= 0 || cache_dirs > 50 || cache_files_per_dir <= 0
                || cache_files_per_dir > 50) {
            fprintf(stderr, "Invalid cache test configuration: dirs=%d, files_per_dir=%d\n",
                    cache_dirs, cache_files_per_dir);
            fatal_failed();
        }

        /* Use dynamic allocation to prevent stack overflow with VLAs */
        char (*cache_test_files)[32] = calloc(cache_dirs * cache_files_per_dir,
                                              sizeof(char[32]));
        int *cache_test_dirs_arr = calloc(cache_dirs, sizeof(int));

        if (!cache_test_files || !cache_test_dirs_arr) {
            fprintf(stderr, "Memory allocation failed for cache test arrays\n");

            if (cache_test_files) {
                free(cache_test_files);
            }

            if (cache_test_dirs_arr) {
                free(cache_test_dirs_arr);
            }

            fatal_failed();
        }

        /* Create test directories */
        for (i = 0; i < cache_dirs; i++) {
            snprintf(temp, sizeof(temp), "cache_dir_%d", i);

            if (!(cache_test_dirs_arr[i] = FPCreateDir(Conn, vol, dir, temp))) {
                free(cache_test_files);
                free(cache_test_dirs_arr);
                fatal_failed();
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
                    fatal_failed();
                }
            }
        }

        /* Now perform many repeated lookups to benefit from caching */
        starttimer();

        for (k = 0; k < cache_lookup_iterations; k++) {
            for (i = 0; i < cache_dirs; i++) {
                /* Directory lookups - is_there() returns AFP_OK (0) when found */
                snprintf(temp, sizeof(temp), "cache_dir_%d", i);

                if (is_there(Conn, dir, temp) != AFP_OK) {
                    fatal_failed();
                }

                /* File parameter requests (should hit cache) */
                for (j = 0; j < cache_files_per_dir; j++) {
                    if (FPGetFileDirParams(Conn, vol, cache_test_dirs_arr[i],
                                           cache_test_files[i * cache_files_per_dir + j],
                                           (1 << FILPBIT_FNUM) | (1 << FILPBIT_PDID) | (1 << FILPBIT_DFLEN), 0)) {
                        fatal_failed();
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
    /* Test (9) - Mixed Cache Operations */
    if (teststorun[TEST_MIXED_CACHE_OPS]) {
        starttimer();

        /* Pattern: create -> stat -> enum -> stat -> delete (tests cache lifecycle) */
        for (i = 0; i < mixed_cache_files; i++) {
            snprintf(temp, sizeof(temp), "mixed_file_%d", i);

            /* Create file */
            if (FPCreateFile(Conn, vol, 0, dir, temp)) {
                fatal_failed();
            }

            /* Stat it (should cache the entry) */
            if (FPGetFileDirParams(Conn, vol, dir, temp,
                                   (1 << FILPBIT_FNUM) | (1 << FILPBIT_PDID) | (1 << FILPBIT_DFLEN), 0)) {
                fatal_failed();
            }

            /* Enumerate directory (should use cached entries) */
            if (i % 10 == 9) {  /* Every 10th iteration */
                if (FPEnumerate(Conn, vol, dir, "", (1 << FILPBIT_LNAME) | (1 << FILPBIT_FNUM),
                                0)) {
                    /* Enumeration failure is not fatal for this test, just log and continue */
                    fprintf(stderr,
                            "Warning: Enumeration failed during mixed cache operations test (iteration %d)\n",
                            i);
                }
            }

            /* Stat again (should hit cache) */
            if (FPGetFileDirParams(Conn, vol, dir, temp,
                                   (1 << FILPBIT_FNUM) | (1 << FILPBIT_CDATE) | (1 << FILPBIT_MDATE), 0)) {
                fatal_failed();
            }

            /* Delete (should invalidate cache entry) */
            if (FPDelete(Conn, vol, dir, temp)) {
                fatal_failed();
            }
        }

        stoptimer();
        addresult(TEST_MIXED_CACHE_OPS, iteration_counter);
    }

    /* -------- */
    /* Test (10) - Deep Path Traversal */
    if (teststorun[TEST_DEEP_TRAVERSAL]) {
        /* Validate configuration to prevent issues */
        if (deep_dir_levels <= 0 || deep_dir_levels > 100) {
            fprintf(stderr, "Invalid deep directory levels: %d\n", deep_dir_levels);
            fatal_failed();
        }

        /* Create a deep directory structure */
        uint32_t *deep_dirs = calloc(deep_dir_levels, sizeof(uint32_t));

        if (!deep_dirs) {
            fprintf(stderr, "Memory allocation failed for deep directories\n");
            fatal_failed();
        }

        uint32_t current_dir = dir;

        /* Create deep directory structure */
        for (i = 0; i < deep_dir_levels; i++) {
            snprintf(temp, sizeof(temp), "deep_%02d", i);

            if (!(deep_dirs[i] = FPCreateDir(Conn, vol, current_dir, temp))) {
                free(deep_dirs);
                fatal_failed();
            }

            current_dir = deep_dirs[i];
        }

        /* Create some files in the deepest directory */
        for (i = 0; i < deep_test_files; i++) {
            snprintf(temp, sizeof(temp), "deep_file_%d", i);

            if (FPCreateFile(Conn, vol, 0, current_dir, temp)) {
                free(deep_dirs);
                fatal_failed();
            }
        }

        starttimer();

        /* Perform deep traversals - this benefits greatly from directory caching */
        for (k = 0; k < deep_traversals; k++) {
            current_dir = dir;

            /* Navigate down the deep structure */
            for (i = 0; i < deep_dir_levels; i++) {
                snprintf(temp, sizeof(temp), "deep_%02d", i);

                /* Directory lookup (should hit cache after first time) - is_there() returns AFP_OK when found */
                if (is_there(Conn, current_dir, temp) != AFP_OK) {
                    free(deep_dirs);
                    fatal_failed();
                }

                current_dir = deep_dirs[i];
            }

            /* Access files in deep directory - limit to avoid excessive operations */
            int files_to_access = (deep_test_files < 10) ? deep_test_files : 10;

            for (i = 0; i < files_to_access; i++) {
                snprintf(temp, sizeof(temp), "deep_file_%d", i);

                if (FPGetFileDirParams(Conn, vol, current_dir, temp,
                                       (1 << FILPBIT_FNUM) | (1 << FILPBIT_DFLEN), 0)) {
                    free(deep_dirs);
                    fatal_failed();
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
    /* Test (11) - Cache Validation Efficiency */
    if (teststorun[TEST_CACHE_VALIDATION]) {
        /* Validate configuration parameters */
        if (validation_files <= 0 || validation_files > 1000 ||
                validation_iterations <= 0 || validation_iterations > 1000) {
            fprintf(stderr,
                    "Invalid validation test configuration: files=%d, iterations=%d\n",
                    validation_files, validation_iterations);
            fatal_failed();
        }

        /* Create test files for validation testing */
        for (i = 0; i < validation_files; i++) {
            snprintf(temp, sizeof(temp), "valid_file_%d", i);

            if (FPCreateFile(Conn, vol, 0, dir, temp)) {
                fatal_failed();
            }
        }

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
                    fatal_failed();
                }

                if (FPGetFileDirParams(Conn, vol, dir, temp,
                                       (1 << FILPBIT_DFLEN) | (1 << FILPBIT_CDATE), 0)) {
                    fatal_failed();
                }

                if (FPGetFileDirParams(Conn, vol, dir, temp,
                                       (1 << FILPBIT_MDATE) | (1 << FILPBIT_FINFO), 0)) {
                    fatal_failed();
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
    int i = 0;
    fprintf(stdout,
            "usage:\t%s [-34567GgVv] [-h host] [-p port] [-s vol] [-u user] [-w password] "
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
    fprintf(stdout, "\t-n\thow many iterations to run (default: 1)\n");
    fprintf(stdout, "\t-v\tverbose\n");
    fprintf(stdout, "\t-V\tvery verbose\n");
    fprintf(stdout, "\t-b\tdebug mode\n");
    fprintf(stdout, "\t-g\tfast network (Gbit, file testsize 1 GB)\n");
    fprintf(stdout,
            "\t-G\tridiculously fast network (10 Gbit, file testsize 10 GB)\n");
    fprintf(stdout, "\t-f\ttests to run, eg 134 for tests 1, 3 and 4\n");
    fprintf(stdout,
            "\t-F\tuse this file located in the volume root for the read test (size must match -g and -G options)\n");
    fprintf(stdout, "\t-C\trun cache-focused tests only (tests 8-11)\n");
    fprintf(stdout, "\tAvailable tests:\n");

    for (i = 0; i < NUMTESTS; i++) {
        fprintf(stdout, "\t(%u) %s", i + 1, resultstrings[i]);

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
int main(int ac, char **av)
{
    int cc, i, t;
    int Debug = 0;
    static char *vers = "AFP3.4";
    static char *uam = "Cleartxt Passwrd";

    if (ac == 1) {
        usage(av[0]);
    }

    while ((cc = getopt(ac, av, "1234567bCGgVvF:f:h:n:p:s:u:w:")) != EOF) {
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

        case 'C':
            /* Special handling for cache tests - set them directly */
            memset(teststorun, 0, NUMTESTS);
            teststorun[TEST_CACHE_HITS] = 1;
            teststorun[TEST_MIXED_CACHE_OPS] = 1;
            teststorun[TEST_DEEP_TRAVERSAL] = 1;
            teststorun[TEST_CACHE_VALIDATION] = 1;
            Test = strdup("C");  /* Mark as cache-only mode */

            if (!Test) {
                fprintf(stderr, "Memory allocation failed for Test string\n");
                exit(1);
            }

            break;

        case 'F':
            bigfilename = strdup(optarg);

            if (!bigfilename) {
                fprintf(stderr, "Memory allocation failed for bigfilename\n");
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
            Iterations = atoi(optarg);

            if (Iterations <= 0) {
                fprintf(stderr, "Error: Number of iterations must be positive (got %d)\n",
                        Iterations);
                exit(1);
            }

            break;

        case 'p' :
            Port = atoi(optarg);

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
            Quiet = 0;
            Verbose = 1;
            break;

        case 'v':
            Quiet = 0;
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
        int ret;
        ret = getpwuid_r(uid, &pwd, buf, sizeof(buf), &result);

        if (ret == 0 && result != NULL) {
            User = pwd.pw_name;
            printf("Using current user: %s\n", User);
        }
    }

    if (!Quiet) {
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

    if (! Debug) {
        if (freopen("/dev/null", "w", stdout) == NULL) {
            fprintf(stderr, "Error: Could not redirect stdout to /dev/null\n");
            exit(1);
        }
    }

#if 0

    /* FIXME: guest auth leads to broken DSI request */
    if ((User[0] == 0) || (Password[0] == 0)) {
        uam = "No User Authent";
    }

#endif
    Iterations_save = Iterations;
    /* Allocate memory for 2D results array: Iterations x NUMTESTS */
    results = calloc(Iterations, sizeof(unsigned long[NUMTESTS]));

    if (!results) {
        fprintf(stderr, "Memory allocation failed for results array\n");
        exit(1);
    }

    if (!Test) {
        memset(teststorun, 1, NUMTESTS);
    } else if (strcmp(Test, "C") == 0) {
        /* Cache-only mode was already set when parsing -C option */
        /* No additional processing needed */
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

        if (Test) {
            free(Test);
        }
    }

    if ((Conn = (CONN *)calloc(1, sizeof(CONN))) == NULL) {
        fprintf(stderr, "Memory allocation failed for connection structure\n");
        return 1;
    }

    int sock;
    dsi = &Conn->dsi;
    sock = OpenClientSocket(Server, Port);

    if (sock < 0) {
        return 2;
    }

    dsi->socket = sock;
    Conn->afp_version = Version;

    /* login */
    if (Version >= 30) {
        ExitCode = ntohs(FPopenLoginExt(Conn, vers, uam, User, Password));
    } else {
        ExitCode = ntohs(FPopenLogin(Conn, vers, uam, User, Password));
    }

    vol  = FPOpenVol(Conn, Vol);

    if (vol == 0xffff) {
        test_nottested();
    }

    int dir;
    char testdir[MAXPATHLEN];
    snprintf(testdir, sizeof(testdir), "LanTest-%d", getpid());

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "", 0, (1 << DIRPBIT_DID))) {
        fatal_failed();
    }

    if (is_there(Conn, DIRDID_ROOT, testdir) == AFP_OK) {
        fatal_failed();
    }

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, testdir))) {
        fatal_failed();
    }

    if (FPGetFileDirParams(Conn, vol, dir, "", 0, (1 << DIRPBIT_DID))) {
        fatal_failed();
    }

    if (is_there(Conn, DIRDID_ROOT, testdir) != AFP_OK) {
        fatal_failed();
    }

    for (int current_iteration = 0; current_iteration < Iterations;
            current_iteration++) {
        /* Update global iteration counter for addresult() calls */
        iteration_counter = current_iteration;
        run_test(dir);
    }

    FPDelete(Conn, vol, dir, "");

    if (ExitCode != AFP_OK && ! Debug) {
        if (!Debug) {
            printf("Error, ExitCode: %u. Run with -v to see what went wrong.\n", ExitCode);
        }

        goto exit;
    }

    displayresults();
exit:

    if (Conn) {
        FPLogOut(Conn);
    }

    return ExitCode;
}
