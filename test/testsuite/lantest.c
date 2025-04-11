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

#include "afpclient.h"
#include "test.h"

#define FPWRITE_RPLY_SIZE 24
#define FPWRITE_RQST_SIZE 36

#define TEST_OPENSTATREAD    0
#define TEST_WRITE100MB      1
#define TEST_READ100MB       2
#define TEST_LOCKUNLOCK      3
#define TEST_CREATE2000FILES 4
#define TEST_ENUM2000FILES   5
#define TEST_DIRTREE         6
#define LASTTEST TEST_DIRTREE
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

static uint16_t vol;
static DSI *dsi;
static char    *Server = "localhost";
static int     Proto = 0;
static int     Port = DSI_AFPOVERTCP_PORT;
static char    *Password = "";
static int     Iterations = 1;
static int     Iterations_save;
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
    "Create directory tree with 10^3 dirs                   "
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
        avg = (rwsize / 1000) / t;
        fprintf(stderr, " for %lu MB (avg. %llu MB/s)", rwsize / (1024 * 1024), avg);
    }
    fprintf(stderr, "\n");
    (*results)[iteration][test] = t;
}

static void displayresults(void)
{
    int i, test, maxindex, minindex, divsub = 0;
    unsigned long long sum, max = 0, min = 18446744073709551615ULL;

    /* Eleminate runaways */
    if (Iterations_save > 5) {
        divsub = 2;
        for (test=0; test != NUMTESTS; test++) {
            if (! teststorun[test])
                continue;
            for (i=0, sum=0; i < Iterations_save; i++) {
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

    fprintf(stderr, "\nNetatalk Lantest Results (averages)\n");
    fprintf(stderr, "===================================\n\n");

    unsigned long long avg, thrput;

    for (test=0; test != NUMTESTS; test++) {
        if (! teststorun[test])
            continue;
        for (i=0, sum=0; i < Iterations_save; i++)
            sum += (*results)[i][test];
        avg = sum / (Iterations_save - divsub);
        if (avg < 1) {
            fprintf(stderr, "\tFATAL ERROR: invalid result\n");
            exit(1);
        }
        if ((test == TEST_WRITE100MB) || (test == TEST_READ100MB)) {
            thrput = (rwsize / 1000) / avg;
            fprintf(stderr, " for %lu MB (avg. %llu MB/s)", rwsize / (1024 * 1024), thrput);
        }

        fprintf(stderr, "\n");
    }

}

/* ------------------------- */
void fatal_failed(void)
{
    fprintf(stdout,"\tFATAL ERROR\n");
    exit(1);
}

/* --------------------------------- */
int is_there(CONN *conn, int did, char *name)
{
    return FPGetFileDirParams(conn, vol,  did, name,
                              (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID)
                              ,
                              (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID)
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

    while (n--) {
        stored = 0;
        while (stored < size) {
            if ((len = recv(dsi->socket, (uint8_t *)buf + stored, size - stored, 0)) == -1 && errno == EINTR)
                continue;
            if (len > 0) {
                stored += len;
            } else {
                fprintf(stdout, "dsi_stream_read(%ld): %s\n", len, (len < 0)?strerror(errno):"EOF");
                goto exit;
            }
        }
    }

exit:
    if (buf)
        free(buf);
    return NULL;
}

/* ------------------------- */
void run_test(const int dir)
{
    static char *data;
    int i,maxi = 0;
    int j, k;
    int fork;
    int test = 0;
    int nowrite;
    off_t offset;
    struct async_io_req air;
    static char temp[MAXPATHLEN];

    numrw = rwsize / (dsi->server_quantum - FPWRITE_RQST_SIZE);

    if (!data)
        data = calloc(1, dsi->server_quantum);

    /* --------------- */
    /* Test (1)        */
    if (teststorun[TEST_OPENSTATREAD]) {
        for (i=0; i <= smallfiles; i++) {
            sprintf(temp, "File.small%d", i);
            if (ntohl(AFPERR_NOOBJ) != is_there(Conn, dir, temp)) {
                fatal_failed();
            }
            if (FPGetFileDirParams(Conn, vol,  dir, "", 0, (1<< DIRPBIT_DID) )) {
                fatal_failed();
            }
            if (FPCreateFile(Conn, vol,  0, dir , temp)){
                fatal_failed();
            }
            if (is_there(Conn, dir, temp)) {
                fatal_failed();
            }
            if (FPGetFileDirParams(Conn, vol,  dir, temp,
                                   (1<<FILPBIT_FNUM )|(1<<FILPBIT_PDID)|(1<<FILPBIT_FINFO)|
                                   (1<<FILPBIT_CDATE)|(1<<FILPBIT_MDATE)|(1<<FILPBIT_DFLEN)|(1<<FILPBIT_RFLEN)
                                   , 0)) {
                fatal_failed();
            }
            if (FPGetFileDirParams(Conn, vol,  dir, temp,
                                   (1<<FILPBIT_FNUM )|(1<<FILPBIT_PDID)|(1<<FILPBIT_FINFO)|
                                   (1<< DIRPBIT_ATTR)|(1<<DIRPBIT_BDATE)|
                                   (1<<FILPBIT_CDATE)|(1<<FILPBIT_MDATE)|(1<<FILPBIT_DFLEN)|(1<<FILPBIT_RFLEN)
                                   , 0)) {
                fatal_failed();
            }
            fork = FPOpenFork(Conn, vol, OPENFORK_DATA ,
                              (1<<FILPBIT_PDID)|(1<< DIRPBIT_LNAME)|(1<<FILPBIT_FNUM)|(1<<FILPBIT_DFLEN)
                              , dir, temp, OPENACC_WR |OPENACC_RD| OPENACC_DWR| OPENACC_DRD);
            if (!fork) {
                fatal_failed();
            }
            if (FPGetForkParam(Conn, fork, (1<<FILPBIT_PDID)|(1<< DIRPBIT_LNAME)|(1<<FILPBIT_DFLEN))) {
                fatal_failed();
            }
            if (FPWrite(Conn, fork, 0, 20480, data, 0 )) {
                fatal_failed();
            }
            if (FPCloseFork(Conn,fork)) {fatal_failed();}
            maxi = i;
        }

        if (FPEnumerate(Conn, vol,  dir , "",
                        (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
                        (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE) |
                        (1<<FILPBIT_DFLEN) | (1<<FILPBIT_RFLEN)
                        ,
                        (1<< DIRPBIT_ATTR) |  (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
                        (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
                        (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
                )) {
            fatal_failed();
        }

        starttimer();
        for (i=1; i <= maxi; i++) {
            sprintf(temp, "File.small%d", i);
            if (is_there(Conn, dir, temp)) {
                fatal_failed();
            }
            if (FPGetFileDirParams(Conn, vol,  dir, temp, 0x72d,0)) {
                fatal_failed();
            }
            if (FPGetFileDirParams(Conn, vol,  dir, temp, 0x73f, 0x133f )) {
                fatal_failed();
            }
            fork = FPOpenFork(Conn, vol, OPENFORK_DATA , 0x342, dir, temp, OPENACC_RD);
            if (!fork) {
                fatal_failed();
            }
            if (FPGetForkParam(Conn, fork, (1<<FILPBIT_DFLEN))) {fatal_failed();}
            if (FPRead(Conn, fork, 0, 512, data)) {fatal_failed();}
            if (FPCloseFork(Conn,fork)) {fatal_failed();}

            fork = FPOpenFork(Conn, vol, OPENFORK_DATA , 0x342 , dir, temp,OPENACC_RD| OPENACC_DWR);
            if (!fork) {
                fatal_failed();
            }
            if (FPGetForkParam(Conn, fork, 0x242)) {fatal_failed();}
            if (FPGetFileDirParams(Conn, vol,  dir, temp, 0x72d,0)) {
                fatal_failed();
            }
            if (FPCloseFork(Conn,fork)) {fatal_failed();}
        }

        stoptimer();
        addresult(TEST_OPENSTATREAD, Iterations);

        /* ---------------- */
        for (i=0; i <= maxi; i++) {
            sprintf(temp, "File.small%d", i);
            if (is_there(Conn, dir, temp)) {
                fatal_failed();
            }
            if (FPGetFileDirParams(Conn, vol,  dir, temp, 0, (1<< FILPBIT_FNUM) )) {
                fatal_failed();
            }
            if (FPDelete(Conn, vol,  dir, temp)) {fatal_failed();}
        }

        if (FPGetVolParam(Conn, vol, (1 << VOLPBIT_MDATE )|(1 << VOLPBIT_XBFREE))) {
            fatal_failed();
        }
    }

    /* -------- */
    /* Test (2) */
    if (teststorun[TEST_WRITE100MB]) {
        strcpy(temp, "File.big");
        if (FPCreateFile(Conn, vol,  0, dir , temp)){
            test_failed();
            goto fin1;
        }

        if (FPGetFileDirParams(Conn, vol,  dir, temp, 0x72d,0))
            fatal_failed();

        if (FPGetFileDirParams(Conn, vol,  dir, temp, 0x73f, 0x133f ))
            fatal_failed();

        fork = FPOpenFork(Conn, vol, OPENFORK_DATA ,
                          (1<<FILPBIT_PDID)|(1<<FILPBIT_FNUM)|(1<<FILPBIT_DFLEN)
                          ,dir, temp, OPENACC_WR |OPENACC_RD| OPENACC_DWR| OPENACC_DRD);
        if (!fork)
            fatal_failed();
        if (FPGetForkParam(Conn, fork, (1<<FILPBIT_PDID)))
            fatal_failed();

        air.air_count = numrw;
        air.air_size = FPWRITE_RPLY_SIZE;
        (void)pthread_create(&tid, NULL, rply_thread, &air);

        starttimer();
        for (i = 0, offset = 0; i < numrw; offset += (dsi->server_quantum - FPWRITE_RQST_SIZE), i++) {
            if (FPWrite_ext_async(Conn, fork, offset, dsi->server_quantum - FPWRITE_RQST_SIZE, data, 0))
                fatal_failed();
        }

        pthread_join(tid, NULL);

        if (FPCloseFork(Conn,fork))
            fatal_failed();

        stoptimer();
        addresult(TEST_WRITE100MB, Iterations);
    }

    /* -------- */
    /* Test (3) */
    if (teststorun[TEST_READ100MB]) {
        if (!bigfilename) {
            if (is_there(Conn, dir, temp) != AFP_OK)
                fatal_failed();

            fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0x342, dir, temp,
                              OPENACC_RD|OPENACC_DWR);
        } else {
            if (is_there(Conn, DIRDID_ROOT, bigfilename) != AFP_OK)
                fatal_failed();

            fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0x342, DIRDID_ROOT, bigfilename,
                              OPENACC_RD|OPENACC_DWR);
        }

        if (!fork)
            fatal_failed();

        if (FPGetForkParam(Conn, fork, 0x242))
            fatal_failed();

        air.air_count = numrw;
        air.air_size = (dsi->server_quantum - FPWRITE_RQST_SIZE) + 16;
        (void)pthread_create(&tid, NULL, rply_thread, &air);

        starttimer();
        for (i=0; i < numrw ; i++) {
            if (FPRead_ext_async(Conn, fork, i * (dsi->server_quantum - FPWRITE_RQST_SIZE), dsi->server_quantum - FPWRITE_RQST_SIZE, data)) {
                fatal_failed();
            }
        }

        pthread_join(tid, NULL);

        if (FPCloseFork(Conn,fork))
            fatal_failed();
        stoptimer();
        addresult(TEST_READ100MB, Iterations);
    }

    /* Remove test 2+3 testfile */
    if (teststorun[TEST_WRITE100MB])
        FPDelete(Conn, vol,  dir, "File.big");

    /* -------- */
    /* Test (4) */
    if (teststorun[TEST_LOCKUNLOCK]) {
        strcpy(temp, "File.lock");
        if (ntohl(AFPERR_NOOBJ) != is_there(Conn, dir, temp))
            fatal_failed();

        if (FPGetFileDirParams(Conn, vol,  dir, "", 0, (1<< DIRPBIT_DID) ))
            fatal_failed();
        if (FPCreateFile(Conn, vol,  0, dir , temp))
            fatal_failed();
        if (is_there(Conn, dir, temp))
            fatal_failed();

        if (FPGetFileDirParams(Conn, vol,  dir, temp,
                               (1<<FILPBIT_FNUM )|(1<<FILPBIT_PDID)|(1<<FILPBIT_FINFO)|
                               (1<<FILPBIT_CDATE)|(1<<FILPBIT_DFLEN)|(1<<FILPBIT_RFLEN)
                               , 0))
            fatal_failed();

        fork = FPOpenFork(Conn, vol, OPENFORK_DATA ,
                          (1<<FILPBIT_PDID)|(1<<FILPBIT_FNUM)|(1<<FILPBIT_DFLEN)
                          , dir, temp, OPENACC_WR|OPENACC_RD|OPENACC_DWR|OPENACC_DRD);
        if (!fork)
            fatal_failed();

        if (FPGetForkParam(Conn, fork, (1<<FILPBIT_PDID)|(1<<FILPBIT_DFLEN)))
            fatal_failed();

        if (FPGetFileDirParams(Conn, vol,  dir, temp,
                               (1<< DIRPBIT_ATTR)|(1<<FILPBIT_CDATE)|(1<<FILPBIT_MDATE)|
                               (1<<FILPBIT_FNUM)|
                               (1<<FILPBIT_FINFO)|(1<<FILPBIT_DFLEN)|(1<<FILPBIT_RFLEN)
                               , 0))
            fatal_failed();

        if (FPWrite(Conn, fork, 0, 40000, data, 0 ))
            fatal_failed();

        if (FPCloseFork(Conn,fork))
            fatal_failed();

        if (is_there(Conn, dir, temp))
            fatal_failed();
        if (FPGetFileDirParams(Conn, vol,  dir, temp, 0x73f, 0x133f))
            fatal_failed();

        fork = FPOpenFork(Conn, vol, OPENFORK_DATA , 0x342 , dir, temp,OPENACC_RD);
        if (!fork)
            fatal_failed();

        if (FPGetForkParam(Conn, fork, (1<<FILPBIT_DFLEN))) {fatal_failed();}
        if (FPRead(Conn, fork, 0, 512, data)) {fatal_failed();}
        if (FPCloseFork(Conn,fork)) {fatal_failed();}

        fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0x342, dir, temp,
                          OPENACC_RD|OPENACC_WR);

        if (!fork)
            fatal_failed();

        if (FPGetForkParam(Conn, fork, 0x242))
            fatal_failed();
        if (FPGetFileDirParams(Conn, vol,  dir, temp, 0x72d, 0))
            fatal_failed();

        starttimer();
        for (j = 0; j < locking; j++) {
            for (i = 0;i <= 390; i += 10) {
                if (FPByteLock(Conn, fork, 0, 0 , i , 10)) {fatal_failed();}
                if (FPByteLock(Conn, fork, 0, 1 , i , 10)) {fatal_failed();}
            }
        }
        stoptimer();
        addresult(TEST_LOCKUNLOCK, Iterations);

        if (is_there(Conn, dir, temp)) {fatal_failed();}
        if (FPCloseFork(Conn,fork)) {fatal_failed();}
        if (FPDelete(Conn, vol,  dir, "File.lock")) {fatal_failed();}
    }

    /* -------- */
    /* Test (5) */
    if (teststorun[TEST_CREATE2000FILES]) {
        starttimer();
        for (i=1; i <= create_enum_files; i++) {
            sprintf(temp, "File.0k%d", i);
            if (FPCreateFile(Conn, vol,  0, dir , temp)){
                fatal_failed();
                break;
            }
            if (FPGetFileDirParams(Conn, vol,  dir, temp,
                                   (1<<FILPBIT_FNUM )|(1<<FILPBIT_PDID)|(1<<FILPBIT_FINFO)|
                                   (1<<FILPBIT_CDATE)|(1<<FILPBIT_DFLEN)|(1<<FILPBIT_RFLEN)
                                   , 0) != AFP_OK)
                fatal_failed();
        }
        maxi = i;

        stoptimer();
        addresult(TEST_CREATE2000FILES, Iterations);
    }

    /* -------- */
    /* Test (6) */
    if (teststorun[TEST_ENUM2000FILES]) {
        starttimer();
        if (FPEnumerateFull(Conn, vol,  1, 40, DSI_DATASIZ, dir , "",
                            (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) |
                            (1<<FILPBIT_FINFO) | (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE)|
                            (1<<FILPBIT_MDATE) | (1<<FILPBIT_DFLEN) | (1<<FILPBIT_RFLEN)
                            ,
                            (1<< DIRPBIT_ATTR) |  (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO)|
                            (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE)|
                            (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|
                            (1<< DIRPBIT_ACCESS)))
            fatal_failed();

        for (i=41; (i + 40) < create_enum_files; i +=80) {
            if (FPEnumerateFull(Conn, vol,  i + 40, 40, DSI_DATASIZ, dir , "",
                                (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) |
                                (1<<FILPBIT_FINFO) | (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE)|
                                (1<<FILPBIT_MDATE) | (1<<FILPBIT_DFLEN) | (1<<FILPBIT_RFLEN)
                                ,
                                (1<< DIRPBIT_ATTR) |  (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO)|
                                (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE)|
                                (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|
                                (1<< DIRPBIT_ACCESS)))
                fatal_failed();
            if (FPEnumerateFull(Conn, vol,  i, 40, DSI_DATASIZ, dir , "",
                                (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) |
                                (1<<FILPBIT_FINFO) | (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE)|
                                (1<<FILPBIT_MDATE) | (1<<FILPBIT_DFLEN) | (1<<FILPBIT_RFLEN)
                                ,
                                (1<< DIRPBIT_ATTR) |  (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO)|
                                (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE)|
                                (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|
                                (1<< DIRPBIT_ACCESS)))
                fatal_failed();
        }
        stoptimer();
        addresult(TEST_ENUM2000FILES, Iterations);
    }

    /* Delete files from Test (5/6) */
    if (teststorun[TEST_CREATE2000FILES]) {
        for (i=1; i < maxi; i++) {
            sprintf(temp, "File.0k%d", i);
            if (FPDelete(Conn, vol, dir, temp))
                fatal_failed();
        }
    }

    /* -------- */
    /* Test (7) */
    if (teststorun[TEST_DIRTREE]) {
        uint32_t idirs[DIRNUM];
        uint32_t jdirs[DIRNUM][DIRNUM];
        uint32_t kdirs[DIRNUM][DIRNUM][DIRNUM];

        starttimer();
        for (i=0; i < DIRNUM; i++) {
            sprintf(temp, "dir%02u", i+1);
            FAILEXIT(!(idirs[i] = FPCreateDir(Conn,vol, dir, temp)), fin1);

            for (j=0; j < DIRNUM; j++) {
                sprintf(temp, "dir%02u", j+1);
                FAILEXIT(!(jdirs[i][j] = FPCreateDir(Conn,vol, idirs[i], temp)), fin1);

                for (k=0; k < DIRNUM; k++) {
                    sprintf(temp, "dir%02u", k+1);
                    FAILEXIT(!(kdirs[i][j][k] = FPCreateDir(Conn,vol, jdirs[i][j], temp)), fin1);
                }
            }
        }
        stoptimer();
        addresult(TEST_DIRTREE, Iterations);

        for (i=0; i < DIRNUM; i++) {
            for (j=0; j < DIRNUM; j++) {
                for (k=0; k < DIRNUM; k++) {
                    FAILEXIT(FPDelete(Conn,vol, kdirs[i][j][k], "") != 0, fin1);
                }
                FAILEXIT(FPDelete(Conn,vol, jdirs[i][j], "") != 0, fin1);
            }
            FAILEXIT(FPDelete(Conn,vol, idirs[i], "") != 0, fin1);
        }
    }

fin1:
fin:
    return;
}

/* =============================== */
void usage( char * av0 )
{
    int i=0;
    fprintf( stdout, "usage:\t%s [-34567GgVv] [-h host] [-p port] [-s vol] [-u user] [-w password] "
    "[-n iterations] [-f tests] [-F bigfile]\n", av0 );
    fprintf( stdout,"\t-h\tserver host name (default localhost)\n");
    fprintf( stdout,"\t-p\tserver port (default 548)\n");
    fprintf( stdout,"\t-s\tvolume to mount\n");
    fprintf( stdout,"\t-u\tuser name (default uid)\n");
    fprintf( stdout,"\t-w\tpassword\n");
    fprintf( stdout,"\t-3\tAFP 3.0 version\n");
    fprintf( stdout,"\t-4\tAFP 3.1 version\n");
    fprintf( stdout,"\t-5\tAFP 3.2 version\n");
    fprintf( stdout,"\t-6\tAFP 3.3 version\n");
    fprintf( stdout,"\t-7\tAFP 3.4 version (default)\n");
    fprintf( stdout,"\t-n\thow many iterations to run (default: 1)\n");
    fprintf( stdout,"\t-v\tverbose\n");
    fprintf( stdout,"\t-V\tvery verbose\n");
    fprintf( stdout,"\t-b\tdebug mode\n");
    fprintf( stdout,"\t-g\tfast network (Gbit, file testsize 1 GB)\n");
    fprintf( stdout,"\t-G\tridiculously fast network (10 Gbit, file testsize 10 GB)\n");
    fprintf( stdout,"\t-f\ttests to run, eg 134 for tests 1, 3 and 4\n");
    fprintf( stdout,"\t-F\tuse this file located in the volume root for the read test (size must match -g and -G options)\n");
    fprintf( stdout,"\tAvailable tests:\n");
    for (i = 0; i < NUMTESTS; i++)
        fprintf( stdout,"\t(%u) %s\n", i+1, resultstrings[i]);
    exit (1);
}

/* ------------------------------- */
int main(int ac, char **av)
{
    int cc, i, t;
    int Debug = 0;
    static char *vers = "AFP3.4";
    static char *uam = "Cleartxt Passwrd";

    while (( cc = getopt( ac, av, "1234567bGgVvF:f:h:n:p:s:u:w:" )) != EOF ) {
        switch ( cc ) {
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
        case 'F':
            bigfilename = strdup(optarg);
            break;
        case 'f':
            Test = strdup(optarg);
            break;
        case 'G':
            rwsize *= 100;
            break;
        case 'g':
            rwsize *= 10;
            break;
        case 'h':
            Server = strdup(optarg);
            break;
        case 'n':
            Iterations = atoi(optarg);
            break;
        case 'p' :
            Port = atoi( optarg );
            if (Port <= 0) {
                fprintf(stdout, "Bad port.\n");
                exit(1);
            }
            break;
        case 's':
            Vol = strdup(optarg);
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
        default :
            usage( av[ 0 ] );
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
        fprintf(stdout, "Error: Define a user with -u\n");
	}
	if (Password != NULL && Password[0] == '\0') {
        fprintf(stdout, "Error: Define a password with -w\n");
	}
	if (Vol != NULL && Vol[0] == '\0') {
        fprintf(stdout, "Error: Define a volume with -s\n");
	}

    if (! Debug) {
        freopen("/dev/null", "w", stdout);
    }

// FIXME: guest auth leads to broken DSI request
#if 0
    if ((User[0] == 0) || (Password[0] == 0))
        uam = "No User Authent";
#endif

    Iterations_save = Iterations;
    results = calloc(Iterations * NUMTESTS, sizeof(unsigned long));

    if (!Test) {
        memset(teststorun, 1, NUMTESTS);
    } else {
        i = 0;
        for (; Test[i]; i++) {
            t = Test[i] - '1';
            if ((t >= 0) && (t <= NUMTESTS))
                teststorun[t] = 1;
        }
        if (teststorun[TEST_READ100MB] && !bigfilename)
            teststorun[TEST_WRITE100MB] = 1;
        if (teststorun[TEST_ENUM2000FILES])
            teststorun[TEST_CREATE2000FILES] = 1;
        free(Test);
    }

    if ((Conn = (CONN *)calloc(1, sizeof(CONN))) == NULL)
        return 1;

    Conn->type = Proto;
    if (!Proto) {
        int sock;
        dsi = &Conn->dsi;
        sock = OpenClientSocket(Server, Port);
        if ( sock < 0) {
            return 2;
        }
        dsi->socket = sock;
    }
    Conn->afp_version = Version;

    /* login */
    if (Version >= 30)
      	ExitCode = ntohs(FPopenLoginExt(Conn, vers, uam, User, Password));
    else
      	ExitCode = ntohs(FPopenLogin(Conn, vers, uam, User, Password));

    vol  = FPOpenVol(Conn, Vol);
    if (vol == 0xffff) {
        test_nottested();
    }

    int dir;
    char testdir[MAXPATHLEN];

    sprintf(testdir, "LanTest-%d", getpid());

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "", 0, (1<< DIRPBIT_DID)))
        fatal_failed();

    if (is_there(Conn, DIRDID_ROOT, testdir) == AFP_OK)
        fatal_failed();

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, testdir)))
        fatal_failed();

    if (FPGetFileDirParams(Conn, vol, dir, "", 0 , (1<< DIRPBIT_DID)))
        fatal_failed();

    if (is_there(Conn, DIRDID_ROOT, testdir) != AFP_OK)
        fatal_failed();

    while (Iterations--) {
        run_test(dir);
    }

    FPDelete(Conn, vol, dir, "");

    if (ExitCode != AFP_OK && ! Debug) {
        if (!Debug)
            printf("Error, ExitCode: %u. Run with -v to see what went wrong.\n", ExitCode);
        goto exit;
    }

    displayresults();

exit:
    if (Conn) FPLogOut(Conn);

    return ExitCode;
}
