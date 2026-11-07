/*
 * Copyright (c) 2026 Andy Lemin (andylemin)
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

/*
 * afp_navtest: Finder-like descent and ascent through a tree of folders that
 * carry custom icons, timing every request. A testing tool for this branch's
 * folder navigation work, removed before review.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "afpclient.h"
#include "afpcmd.h"
#include "afphelper.h"
#include "afptest_uam.h"
#include "testhelper.h"

/* linker names the client library expects */
CONN *Conn;
CONN *Conn2;
int ExitCode = 0;
int Version = 34;
int Mac = 0;
int EmptyVol = 0;
char Data[300000] = "";
char *Vol = "";
char *User = "";
char *Path;
char *Test = NULL;
char *Server = "127.0.0.1";
uint16_t VolID;
int PassCount = 0;
int FailCount = 0;
int SkipCount = 0;
int NotTestedCount = 0;
char FailedTests[1024][256] = {{0}};
char NotTestedTests[1024][256] = {{0}};
char SkippedTests[1024][256] = {{0}};

static int Port = 548;
static int Port2 = 0;
static char *Password = "";
static int Depth = 6;
static int Width = 20;
static int RsrcLen = 1024;
static bool WriteDSStore = false;
static bool SignalDumps = false;
static bool CreateOnly = false;
static bool NavigateOnly = false;
static bool RemoveOnly = false;
static pid_t Session;           /* the navigation session's afpd, or 0 */
static bool Markers = false;
static bool ByPath = false;
static const char *Uam = NULL;
static int Rounds = 1;
static int BenchReps = 0;

#define MAXD 32
#define MAXW 256
#define MAXR 8192               /* the icon bytes a read takes, at most */
#define MAXS 1024               /* the afpd sessions a login looks through */
#define MARKER ".afp_navtest"   /* -x removes a top folder holding this file */
#define ICON_NAME "Icon\r"

static uint32_t folder_did[MAXD + 1];
static uint32_t child_did[MAXD][MAXW];

#define DBITS ((1 << DIRPBIT_ATTR) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_CDATE) \
    | (1 << DIRPBIT_MDATE) | (1 << DIRPBIT_BDATE) | (1 << DIRPBIT_FINFO) \
    | (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_DID) | (1 << DIRPBIT_OFFCNT) \
    | (1 << DIRPBIT_UID) | (1 << DIRPBIT_GID) | (1 << DIRPBIT_ACCESS) \
    | (1 << DIRPBIT_PDINFO) | (1 << DIRPBIT_UNIXPR))
#define FBITS ((1 << FILPBIT_ATTR) | (1 << FILPBIT_PDID) | (1 << FILPBIT_CDATE) \
    | (1 << FILPBIT_MDATE) | (1 << FILPBIT_BDATE) | (1 << FILPBIT_FINFO) \
    | (1 << FILPBIT_LNAME) | (1 << FILPBIT_FNUM) | (1 << FILPBIT_DFLEN) \
    | (1 << FILPBIT_RFLEN) | (1 << FILPBIT_EXTDFLEN) | (1 << FILPBIT_PDINFO) \
    | (1 << FILPBIT_EXTRFLEN) | (1 << FILPBIT_UNIXPR))

/*! @brief The monotonic clock in microseconds */
static double now_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e6 + (double)ts.tv_nsec / 1e3;
}

/*! @brief Report a failed request or call and exit with status 1 */
static void die(const char *what, unsigned int ret)
{
    fprintf(stderr, "navtest: %s failed: 0x%x (%s)\n", what, ret,
            afp_error(ret));
    exit(1);
}

/*! @brief Connect to the server and log in with the chosen UAM */
static CONN *connect_login(void)
{
    CONN *c = calloc(1, sizeof(CONN));
    char *vers = "AFP3.4";
    char *uam = "Cleartxt Passwrd";
    unsigned int ret;

    if (c == NULL) {
        die("calloc", 0);
    }

    int sock = OpenClientSocket(Server, Port);

    if (sock < 0) {
        die("OpenClientSocket", 0);
    }

    c->dsi.socket = sock;
    c->afp_version = Version;
    ret = Uam != NULL
          ? ntohs((uint16_t)afptest_uam_login(c, vers, Uam, User, Password))
          : ntohs((uint16_t)FPopenLoginExt(c, vers, uam, User, Password));

    if (ret != 0) {
        die("login", ret);
    }

    return c;
}

/*! @brief Close the volume and the session */
static void disconnect(CONN *c, uint16_t vol)
{
    FPCloseVol(c, vol);
    FPLogOut(c);
    CloseClientSocket(c->dsi.socket);
    free(c);
}

/* the afpd session children, afpds whose parent is an afpd, into pids; none
 * where there is no Linux /proc */
static int afpd_children(pid_t *pids, int max)
{
    DIR *dp = opendir("/proc");
    struct dirent *de;
    int n = 0;

    if (dp == NULL) {
        return 0;
    }

    while (n < max && (de = readdir(dp)) != NULL) {
        char path[64], comm[64], pcomm[64];
        pid_t pid, ppid;
        FILE *f;

        if (!isdigit((unsigned char)de->d_name[0])) {
            continue;
        }

        snprintf(path, sizeof(path), "/proc/%s/stat", de->d_name);

        if ((f = fopen(path, "r")) == NULL) {
            continue;
        }

        if (fscanf(f, "%d (%63[^)]) %*c %d", &pid, comm, &ppid) != 3) {
            fclose(f);
            continue;
        }

        fclose(f);

        if (strcmp(comm, "afpd") != 0) {
            continue;
        }

        snprintf(path, sizeof(path), "/proc/%d/stat", ppid);

        if ((f = fopen(path, "r")) == NULL) {
            continue;
        }

        if (fscanf(f, "%*d (%63[^)])", pcomm) == 1 && strcmp(pcomm, "afpd") == 0) {
            pids[n++] = pid;
        }

        fclose(f);
    }

    closedir(dp);
    return n;
}

/*! @brief Whether -h names this host, whose /proc shows afpd's children */
static bool local_server(void)
{
    return Server != NULL && (strcmp(Server, "localhost") == 0
                              || strncmp(Server, "127.", 4) == 0
                              || strcmp(Server, "::1") == 0);
}

/* log in, and name the afpd session child the login made: the one that was
 * not there before, or 0 where none or more than one appeared, or the
 * server is another host */
static CONN *connect_session(pid_t *session)
{
    pid_t before[MAXS];
    pid_t after[MAXS];
    const int nb = afpd_children(before, MAXS);
    CONN *c = connect_login();
    const int na = afpd_children(after, MAXS);
    pid_t found = 0;
    int fresh = 0;

    for (int i = 0; i < na; i++) {
        bool seen = false;

        for (int j = 0; j < nb && !seen; j++) {
            seen = after[i] == before[j];
        }

        if (!seen) {
            found = after[i];
            fresh++;
        }
    }

    /* /proc shows the children of this host's afpd only */
    *session = fresh == 1 && local_server() ? found : 0;
    return c;
}

/*! @brief With -S, ask the session to dump its dircache, tagged in stdout */
static void dump_request(const char *tag)
{
    pid_t pid;

    if (!SignalDumps) {
        return;
    }

    pid = Session;

    if (pid <= 0) {
        fprintf(stderr, "navtest: no afpd session known for %s: no Linux "
                        "/proc, or another session started with it\n", tag);
        return;
    }

    printf("## SIGINT -> afpd child %d (%s)\n", (int)pid, tag);
    fflush(stdout);

    if (kill(pid, SIGINT) != 0) {
        perror("navtest: kill(afpd, SIGINT)");
        return;
    }

    /* the dump runs when the next request wakes the main loop */
    const struct timespec pause = { 0, 200000000 };
    nanosleep(&pause, NULL);
}

static void make_icon(CONN *c, uint16_t vol, uint32_t did, uint32_t pdid,
                      char *name)
{
    struct afp_filedir_parms fd;
    unsigned int ret;
    uint16_t frflags;
    uint16_t fork;

    if ((ret = FPCreateFile(c, vol, 0, did, ICON_NAME)) != 0) {
        die("FPCreateFile Icon", ret);
    }

    fork = FPOpenFork(c, vol, OPENFORK_RSCS, 0, did, ICON_NAME,
                      OPENACC_WR | OPENACC_RD);

    if (fork == 0) {
        die("FPOpenFork rsrc write", c->dsi.header.dsi_code);
    }

    if ((ret = FPWrite(c, fork, 0, RsrcLen, Data, 0)) != 0) {
        die("FPWrite rsrc", ret);
    }

    if ((ret = FPCloseFork(c, fork)) != 0) {
        die("FPCloseFork", ret);
    }

    /* the folder carries kHasCustomIcon, as the Finder sets it */
    memset(&fd, 0, sizeof(fd));
    fd.isdir = 1;
    frflags = htons(FINDERINFO_HASCUSTOMICON);
    memcpy(fd.finder_info + 8, &frflags, sizeof(frflags));

    if ((ret = FPSetDirParms(c, vol, pdid, name, 1 << DIRPBIT_FINFO, &fd)) != 0) {
        die("FPSetDirParms FINFO", ret);
    }

    /* the Icon file is invisible */
    memset(&fd, 0, sizeof(fd));
    frflags = htons(0x4000);
    memcpy(fd.finder_info + 8, &frflags, sizeof(frflags));

    if ((ret = FPSetFileParams(c, vol, did, ICON_NAME, 1 << FILPBIT_FINFO,
                               &fd)) != 0) {
        die("FPSetFileParams FINFO", ret);
    }
}

/*! @brief Make the tree: Width icon folders at each of Depth levels */
static void create_tree(CONN *c, uint16_t vol)
{
    folder_did[0] = DIRDID_ROOT;

    for (int L = 0; L < Depth; L++) {
        for (int i = 0; i < Width; i++) {
            char name[64];
            uint32_t did;
            snprintf(name, sizeof(name), "L%d_d%02d", L + 1, i);
            did = FPCreateDir(c, vol, folder_did[L], name);

            if (did == 0) {
                fprintf(stderr, "navtest: %s could not be made; -x removes "
                                "a tree an earlier run left\n", name);
                die("FPCreateDir", c->dsi.header.dsi_code);
            }

            child_did[L][i] = did;
            make_icon(c, vol, did, folder_did[L], name);

            /* the marker -x looks for before it removes a top folder */
            if (L == 0) {
                unsigned int ret = FPCreateFile(c, vol, 0, did, MARKER);

                if (ret != 0) {
                    die("FPCreateFile " MARKER, ret);
                }
            }
        }

        folder_did[L + 1] = child_did[L][0];
    }

    printf("## tree created: depth %d, width %d, path DIDs:", Depth, Width);

    for (int L = 0; L <= Depth; L++) {
        printf(" %u", ntohl(folder_did[L]));
    }

    printf("\n");
}

/* recover the DIDs of a tree another session created */
static void learn_tree(CONN *c, uint16_t vol)
{
    const DSI *dsi = &c->dsi;
    folder_did[0] = DIRDID_ROOT;

    for (int L = 0; L < Depth; L++) {
        for (int i = 0; i < Width; i++) {
            char name[64];
            struct afp_filedir_parms fd = { 0 };
            snprintf(name, sizeof(name), "L%d_d%02d", L + 1, i);

            if (FPGetFileDirParams(c, vol, folder_did[L], name, 0,
                                   1 << DIRPBIT_DID) != 0) {
                die("FPGetFileDirParams (learn)", dsi->header.dsi_code);
            }

            fd.isdir = 1;
            afp_filedir_unpack(c, &fd, (const unsigned char *)dsi->data + 6, 0,
                               1 << DIRPBIT_DID);
            child_did[L][i] = fd.did;
        }

        folder_did[L + 1] = child_did[L][0];
    }
}

/*! @brief Write a 6148-byte .DS_Store into the folder, as the Finder does */
static void write_dsstore(CONN *c, uint16_t vol, uint32_t did)
{
    unsigned int ret = FPCreateFile(c, vol, 0, did, ".DS_Store");
    uint16_t fork;

    if (ret != 0 && ret != (unsigned int)ntohl(AFPERR_EXIST)) {
        die("FPCreateFile .DS_Store", ret);
    }

    fork = FPOpenFork(c, vol, OPENFORK_DATA, 0, did, ".DS_Store",
                      OPENACC_WR | OPENACC_RD);

    if (fork == 0) {
        die("FPOpenFork .DS_Store", c->dsi.header.dsi_code);
    }

    if ((ret = FPWrite(c, fork, 0, 6148, Data, 0)) != 0) {
        die("FPWrite .DS_Store", ret);
    }

    if ((ret = FPCloseFork(c, fork)) != 0) {
        die("FPCloseFork .DS_Store", ret);
    }
}

/* one Finder window over folder k of the path */
static void display(CONN *c, uint16_t vol, int k, const char *phase)
{
    const DSI *dsi = &c->dsi;
    uint32_t did;
    int nchildren = k < Depth ? Width : 0;
    double t0, t1, t2, t3, tmax = 0, treq;
    unsigned int ret;
    uint32_t sindex = 1;
    int batches = 0, listed = 0;
    char rbuf[MAXR];
    char path[MAXD * 16] = "";

    if (k < 0 || k > MAXD) {
        return;
    }

    did = folder_did[k];
    t0 = now_us();

    /* -P names the folder by its path from the root, as a client that
     * resolves paths sends it; '/' becomes the AFP separator */
    for (int L = 1; ByPath && L <= k; L++) {
        size_t used = strlen(path);
        snprintf(path + used, sizeof(path) - used, "%sL%d_d00", L > 1 ? "/" : "",
                 L);
    }

    if ((ret = FPGetFileDirParams(c, vol, ByPath ? DIRDID_ROOT : did, path, 0,
                                  DBITS)) != 0) {
        die("FPGetFileDirParams folder", ret);
    }

    t1 = now_us();

    for (;;) {
        uint16_t actcnt;
        ret = FPEnumerateExt2Full(c, vol, did, "", FBITS, DBITS, sindex, 1000);

        if (ret == (unsigned int)ntohl(AFPERR_NOOBJ)) {
            break;
        }

        if (ret != 0) {
            die("FPEnumerateExt2", ret);
        }

        memcpy(&actcnt, dsi->data + 4, sizeof(actcnt));
        actcnt = ntohs(actcnt);
        listed += actcnt;
        sindex += actcnt;
        batches++;

        if (actcnt == 0) {
            break;
        }
    }

    t2 = now_us();

    for (int i = 0; i < nchildren; i++) {
        uint32_t cdid = child_did[k][i];
        uint16_t fork;
        double ts = now_us();

        if ((ret = FPGetFileDirParams(c, vol, cdid, ICON_NAME, FBITS, 0)) != 0) {
            die("FPGetFileDirParams Icon", ret);
        }

        treq = now_us() - ts;
        tmax = treq > tmax ? treq : tmax;
        ts = now_us();
        fork = FPOpenFork(c, vol, OPENFORK_RSCS, 0, cdid, ICON_NAME, OPENACC_RD);

        if (fork == 0) {
            die("FPOpenFork Icon rsrc", dsi->header.dsi_code);
        }

        treq = now_us() - ts;
        tmax = treq > tmax ? treq : tmax;
        ts = now_us();

        /* a fork shorter than -R, from an earlier -c, reads to its end */
        if ((ret = FPRead(c, fork, 0, RsrcLen, rbuf)) != 0
                && ret != (unsigned int)ntohl(AFPERR_EOF)) {
            die("FPRead Icon rsrc", ret);
        }

        treq = now_us() - ts;
        tmax = treq > tmax ? treq : tmax;
        ts = now_us();

        if ((ret = FPCloseFork(c, fork)) != 0) {
            die("FPCloseFork Icon rsrc", ret);
        }

        treq = now_us() - ts;
        tmax = treq > tmax ? treq : tmax;
    }

    t3 = now_us();
    printf("%-4s k=%d did=%-6u getparms=%7.0f  enum=%7.0f (%d listed, %d batches)  "
           "icons=%8.0f (n=%d, max req %6.0f)  total=%8.0f us\n",
           phase, k, ntohl(did), t1 - t0, t2 - t1, listed, batches, t3 - t2,
           nchildren, tmax, t3 - t0);
    fflush(stdout);

    if (WriteDSStore) {
        write_dsstore(c, vol, did);
    }
}

/*! @brief qsort() order of two doubles */
static int cmp_double(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return x < y ? -1 : x > y;
}

/* the q-quantile of n sorted samples */
static double quantile(const double *v, int n, double q)
{
    int i = (int)(q * (n - 1) + 0.5);
    return v[i < 0 ? 0 : i >= n ? n - 1 : i];
}

/* a counter's growth over n repetitions, per repetition; NAN where a
 * process gone between the samples read as 0 */
static double per_rep(unsigned long long after, unsigned long long before,
                      double n)
{
    return after >= before ? (double)(after - before) / n : NAN;
}

/* run and run-queue wait time, in ns, of every thread of pid (schedstat) */
static void proc_times(pid_t pid, unsigned long long *run,
                       unsigned long long *wait)
{
    char dir[64];
    DIR *dp;
    const struct dirent *de;
    *run = 0;
    *wait = 0;
    snprintf(dir, sizeof(dir), "/proc/%d/task", (int)pid);

    if (pid <= 0 || (dp = opendir(dir)) == NULL) {
        return;
    }

    while ((de = readdir(dp)) != NULL) {
        char f[128];
        unsigned long long r, w;
        FILE *fp;

        if (!isdigit((unsigned char)de->d_name[0])) {
            continue;
        }

        snprintf(f, sizeof(f), "%s/%s/schedstat", dir, de->d_name);

        if ((fp = fopen(f, "r")) == NULL) {
            continue;
        }

        if (fscanf(fp, "%llu %llu", &r, &w) == 2) {
            *run += r;
            *wait += w;
        }

        fclose(fp);
    }

    closedir(dp);
}

/* a numeric option between lo and hi, or exit 2 */
static long num_opt(const char *s, long lo, long hi, const char *what)
{
    char *end;
    long v;
    errno = 0;
    v = strtol(s, &end, 10);

    if (*s == '\0' || *end != '\0' || errno != 0 || v < lo || v > hi) {
        fprintf(stderr, "navtest: %s takes a number %ld..%ld\n", what, lo,
                hi);
        exit(2);
    }

    return v;
}

/* the last /proc entry whose comm is name, or 0; where several run, the
 * NAV_FUSE variables name the one that serves the volume */
static pid_t pid_by_comm(const char *name)
{
    DIR *dp = opendir("/proc");
    const struct dirent *de;
    pid_t found = 0;

    if (dp == NULL) {
        return 0;
    }

    while ((de = readdir(dp)) != NULL) {
        char path[64], comm[64];
        FILE *f;

        if (!isdigit((unsigned char)de->d_name[0])) {
            continue;
        }

        snprintf(path, sizeof(path), "/proc/%s/comm", de->d_name);

        if ((f = fopen(path, "r")) == NULL) {
            continue;
        }

        if (fgets(comm, sizeof(comm), f) != NULL) {
            char *nl = strchr(comm, '\n');

            if (nl != NULL) {
                *nl = '\0';
            }

            if (strcmp(comm, name) == 0) {
                found = (pid_t)atoi(de->d_name);
            }
        }

        fclose(f);
    }

    closedir(dp);
    return found;
}

/*! @brief The folder at depth k, by its path from the root or by its DID */
static unsigned int bench_req(CONN *c, uint16_t vol, int k, char *path,
                              bool by_path)
{
    return by_path
           ? FPGetFileDirParams(c, vol, DIRDID_ROOT, path, 0, DBITS)
           : FPGetFileDirParams(c, vol, folder_did[k], "", 0, DBITS);
}

/*!
 * @brief -B reps: at each depth, alternate a request naming the folder by
 *        its path from the root with one naming it by its DID
 *
 * Both answer the same folder's parameters, so their difference is what
 * resolving the path costs; the order swaps each repetition, so neither kind
 * always follows a request that just walked the same folder. Then a batch of
 * each kind measures the CPU time afpd, and the FUSE daemon when one serves
 * the volume, spent per request. In a VM that reports no steal time, the
 * host's pre-emption counts as run time too; -q pairs two servers so that it
 * falls on both alike.
 */
static void bench(CONN *c, uint16_t vol)
{
    double *tp = calloc((size_t)BenchReps, sizeof(double));
    double *td = calloc((size_t)BenchReps, sizeof(double));
    char path[MAXD * 16] = "";
    unsigned int ret;
    const char *fuse_env = getenv("NAV_FUSE");
    pid_t afpd = Session;
    pid_t fuse = fuse_env != NULL ? (pid_t)num_opt(fuse_env, 1, INT_MAX,
                                                   "NAV_FUSE")
                 : pid_by_comm("fuse-overlayfs");

    if (tp == NULL || td == NULL) {
        die("calloc", 0);
    }

    printf("## bench: %d requests of each kind per depth, alternating; "
           "afpd session %d, FUSE daemon %d\n", BenchReps, (int)afpd,
           (int)fuse);

    for (int k = 1; k <= Depth; k++) {
        size_t used = strlen(path);
        snprintf(path + used, sizeof(path) - used, "%sL%d_d00", k > 1 ? "/" : "",
                 k);

        /* one of each first, so both start from a warm cache */
        if ((ret = FPGetFileDirParams(c, vol, DIRDID_ROOT, path, 0, DBITS)) != 0
                || (ret = FPGetFileDirParams(c, vol, folder_did[k], "", 0,
                                             DBITS)) != 0) {
            die("bench warm-up", ret);
        }

        for (int i = 0; i < BenchReps; i++) {
            const bool path_first = (i & 1) == 0;
            double *first = path_first ? &tp[i] : &td[i];
            double *second = path_first ? &td[i] : &tp[i];
            double t0 = now_us();
            double t1;

            if ((ret = bench_req(c, vol, k, path, path_first)) != 0) {
                die(path_first ? "bench path" : "bench DID", ret);
            }

            t1 = now_us();

            if ((ret = bench_req(c, vol, k, path, !path_first)) != 0) {
                die(path_first ? "bench DID" : "bench path", ret);
            }

            *first = t1 - t0;
            *second = now_us() - t1;
        }

        qsort(tp, (size_t)BenchReps, sizeof(double), cmp_double);
        qsort(td, (size_t)BenchReps, sizeof(double), cmp_double);
        printf("BENCH depth=%d path_p10=%.1f path_med=%.1f path_p90=%.1f "
               "did_p10=%.1f did_med=%.1f did_p90=%.1f\n", k,
               quantile(tp, BenchReps, 0.1), quantile(tp, BenchReps, 0.5),
               quantile(tp, BenchReps, 0.9), quantile(td, BenchReps, 0.1),
               quantile(td, BenchReps, 0.5), quantile(td, BenchReps, 0.9));

        if (afpd > 0) {
            unsigned long long ar[3], aw[3], fr[3], fw[3];
            double n = (double)BenchReps;
            proc_times(afpd, &ar[0], &aw[0]);
            proc_times(fuse, &fr[0], &fw[0]);

            for (int i = 0; i < BenchReps; i++) {
                if ((ret = FPGetFileDirParams(c, vol, DIRDID_ROOT, path, 0,
                                              DBITS)) != 0) {
                    die("bench path batch", ret);
                }
            }

            proc_times(afpd, &ar[1], &aw[1]);
            proc_times(fuse, &fr[1], &fw[1]);

            for (int i = 0; i < BenchReps; i++) {
                if ((ret = FPGetFileDirParams(c, vol, folder_did[k], "", 0,
                                              DBITS)) != 0) {
                    die("bench DID batch", ret);
                }
            }

            proc_times(afpd, &ar[2], &aw[2]);
            proc_times(fuse, &fr[2], &fw[2]);
            printf("BENCHCPU depth=%d path_afpd_ns=%.0f did_afpd_ns=%.0f "
                   "path_wait_ns=%.0f did_wait_ns=%.0f path_fuse_ns=%.0f "
                   "did_fuse_ns=%.0f\n", k, per_rep(ar[1], ar[0], n),
                   per_rep(ar[2], ar[1], n), per_rep(aw[1], aw[0], n),
                   per_rep(aw[2], aw[1], n), per_rep(fr[1], fr[0], n),
                   per_rep(fr[2], fr[1], n));
        }

        fflush(stdout);
    }

    free(tp);
    free(td);
}

/* one server of the paired bench */
struct side {
    CONN *c;
    uint16_t vol;
    uint32_t did[MAXD + 1];
    pid_t afpd;
    pid_t fuse;
    double *tp;
    double *td;
    double *cp;
    double *cd;
};

/* the parent of pid, or 0 */
static pid_t ppid_of(pid_t pid)
{
    char path[64];
    FILE *f;
    int ppid = 0;
    snprintf(path, sizeof(path), "/proc/%d/stat", (int)pid);

    if (pid <= 0 || (f = fopen(path, "r")) == NULL) {
        return 0;
    }

    if (fscanf(f, "%*d (%*[^)]) %*c %d", &ppid) != 1) {
        ppid = 0;
    }

    fclose(f);
    return (pid_t)ppid;
}

/* run and wait times of a side's afpd session and FUSE daemon */
static void side_times(const struct side *x, unsigned long long t[4])
{
    proc_times(x->afpd, &t[0], &t[1]);
    proc_times(x->fuse, &t[2], &t[3]);
}

/*! @brief Open one side of a paired benchmark: its session, volume, tree
 *         DIDs, FUSE daemon from fuse_env, and timing arrays */
static void side_open(struct side *s, int port, const char *fuse_env)
{
    const char *f = getenv(fuse_env);
    Port = port;
    s->c = connect_session(&s->afpd);
    s->vol = FPOpenVol(s->c, Vol);

    if (s->vol == 0xffff) {
        die("FPOpenVol", 0);
    }

    learn_tree(s->c, s->vol);
    memcpy(s->did, folder_did, sizeof(s->did));
    s->fuse = f != NULL ? (pid_t)num_opt(f, 1, INT_MAX, fuse_env) : 0;
    s->tp = calloc((size_t)BenchReps, sizeof(double));
    s->td = calloc((size_t)BenchReps, sizeof(double));
    s->cp = calloc((size_t)BenchReps, sizeof(double));
    s->cd = calloc((size_t)BenchReps, sizeof(double));

    if (s->tp == NULL || s->td == NULL || s->cp == NULL || s->cd == NULL) {
        die("calloc", 0);
    }
}

/* the run time, in us, of a side's afpd session and FUSE daemon; read
 * between requests, while both sleep and their counts are current */
static double side_run(const struct side *x)
{
    unsigned long long t[4];
    side_times(x, t);
    return (double)(t[0] + t[2]) / 1e3;
}

/* the folder at depth k, by its path from the root or by its DID */
static unsigned int side_req(const struct side *x, char *path, int k,
                             bool by_path)
{
    return by_path
           ? FPGetFileDirParams(x->c, x->vol, DIRDID_ROOT, path, 0, DBITS)
           : FPGetFileDirParams(x->c, x->vol, x->did[k], "", 0, DBITS);
}

/*
 * -B reps -q port2: the -B requests against the server on -p, side A, and
 * the one on -q, side B. Each iteration sends its path and DID pair to one
 * side and then the other, the first alternating. The host pre-empts the
 * VM's CPUs when it likes, and the VM accounts that time to whatever thread
 * it stopped; with both servers on one CPU and their requests microseconds
 * apart, that load falls on both alike. dcost is the per-iteration
 * difference of their path costs, B's minus A's, and dcpu the same of the
 * run time each request cost afpd and the FUSE daemon; a median of those
 * leaves out the requests the host stopped, which a batch's mean keeps.
 */
static void bench_pair(void)
{
    struct side s[2];
    char path[MAXD * 16] = "";
    double *dc = calloc((size_t)BenchReps, sizeof(double));
    double *du = calloc((size_t)BenchReps, sizeof(double));
    const int port[2] = { Port, Port2 };
    unsigned int ret;

    if (dc == NULL || du == NULL) {
        die("calloc", 0);
    }

    side_open(&s[0], port[0], "NAV_FUSE_A");
    side_open(&s[1], port[1], "NAV_FUSE_B");
    /* without the sessions in view, as off Linux, the CPU fields read 0 */
    const bool cpu = s[0].afpd > 0 && s[1].afpd > 0;
    printf("## pair bench: %d requests of each kind per depth and side, "
           "alternating%s\n", BenchReps,
           cpu ? "" : "; no afpd session in view, so no CPU times");

    for (int j = 0; j < 2; j++) {
        printf("## side %c: port %d, afpd session %d of master %d, "
               "FUSE daemon %d\n", 'A' + j, port[j], (int)s[j].afpd,
               (int)ppid_of(s[j].afpd), (int)s[j].fuse);
    }

    if (cpu && ppid_of(s[0].afpd) == ppid_of(s[1].afpd)) {
        die("pair: a session on each server", 0);
    }

    for (int k = 1; k <= Depth; k++) {
        size_t used = strlen(path);
        unsigned long long t[2][3][4];
        double n = (double)BenchReps;
        snprintf(path + used, sizeof(path) - used, "%sL%d_d00", k > 1 ? "/" : "",
                 k);

        for (int j = 0; j < 2; j++) {
            if ((ret = side_req(&s[j], path, k, true)) != 0
                    || (ret = side_req(&s[j], path, k, false)) != 0) {
                die("pair warm-up", ret);
            }
        }

        for (int i = 0; i < BenchReps; i++) {
            for (int j = 0; j < 2; j++) {
                struct side *x = &s[(i + j) & 1];
                double c0 = side_run(x);
                double t0 = now_us();

                if ((ret = side_req(x, path, k, true)) != 0) {
                    die("pair path", ret);
                }

                double t1 = now_us();
                double c1 = side_run(x);
                double t2 = now_us();

                if ((ret = side_req(x, path, k, false)) != 0) {
                    die("pair DID", ret);
                }

                double t3 = now_us();
                double c2 = side_run(x);
                x->tp[i] = t1 - t0;
                x->td[i] = t3 - t2;
                x->cp[i] = c1 - c0;
                x->cd[i] = c2 - c1;
            }

            dc[i] = (s[1].tp[i] - s[1].td[i]) - (s[0].tp[i] - s[0].td[i]);
            du[i] = (s[1].cp[i] - s[1].cd[i]) - (s[0].cp[i] - s[0].cd[i]);
        }

        qsort(dc, (size_t)BenchReps, sizeof(double), cmp_double);
        qsort(du, (size_t)BenchReps, sizeof(double), cmp_double);

        for (int j = 0; j < 2; j++) {
            qsort(s[j].tp, (size_t)BenchReps, sizeof(double), cmp_double);
            qsort(s[j].td, (size_t)BenchReps, sizeof(double), cmp_double);
            qsort(s[j].cp, (size_t)BenchReps, sizeof(double), cmp_double);
            qsort(s[j].cd, (size_t)BenchReps, sizeof(double), cmp_double);
        }

        printf("PAIR depth=%d", k);

        for (int j = 0; j < 2; j++) {
            printf(" %c_path_p10=%.1f %c_path_med=%.1f %c_did_p10=%.1f "
                   "%c_did_med=%.1f %c_cpath_med=%.2f %c_cdid_med=%.2f",
                   'a' + j, quantile(s[j].tp, BenchReps, 0.1),
                   'a' + j, quantile(s[j].tp, BenchReps, 0.5), 'a' + j,
                   quantile(s[j].td, BenchReps, 0.1), 'a' + j,
                   quantile(s[j].td, BenchReps, 0.5), 'a' + j,
                   quantile(s[j].cp, BenchReps, 0.5), 'a' + j,
                   quantile(s[j].cd, BenchReps, 0.5));
        }

        printf(" dcost_p10=%.1f dcost_med=%.1f dcost_p90=%.1f "
               "dcpu_p10=%.2f dcpu_med=%.2f dcpu_p90=%.2f\n",
               quantile(dc, BenchReps, 0.1), quantile(dc, BenchReps, 0.5),
               quantile(dc, BenchReps, 0.9), quantile(du, BenchReps, 0.1),
               quantile(du, BenchReps, 0.5), quantile(du, BenchReps, 0.9));

        if (!cpu) {
            fflush(stdout);
            continue;
        }

        /* CPU: a batch of path requests, then one of DID requests, each
         * alternating the sides; each side's times count its requests only */
        for (int j = 0; j < 2; j++) {
            side_times(&s[j], t[j][0]);
        }

        for (int i = 0; i < BenchReps; i++) {
            for (int j = 0; j < 2; j++) {
                if ((ret = side_req(&s[(i + j) & 1], path, k, true)) != 0) {
                    die("pair path batch", ret);
                }
            }
        }

        for (int j = 0; j < 2; j++) {
            side_times(&s[j], t[j][1]);
        }

        for (int i = 0; i < BenchReps; i++) {
            for (int j = 0; j < 2; j++) {
                if ((ret = side_req(&s[(i + j) & 1], path, k, false)) != 0) {
                    die("pair DID batch", ret);
                }
            }
        }

        for (int j = 0; j < 2; j++) {
            side_times(&s[j], t[j][2]);
        }

        printf("PAIRCPU depth=%d", k);

        for (int j = 0; j < 2; j++) {
            const unsigned long long *b = t[j][0], *m = t[j][1], *e = t[j][2];
            printf(" %c_path_ns=%.0f %c_did_ns=%.0f %c_path_wait_ns=%.0f "
                   "%c_did_wait_ns=%.0f %c_path_fuse_ns=%.0f %c_did_fuse_ns=%.0f",
                   'a' + j, per_rep(m[0], b[0], n),
                   'a' + j, per_rep(e[0], m[0], n),
                   'a' + j, per_rep(m[1], b[1], n),
                   'a' + j, per_rep(e[1], m[1], n),
                   'a' + j, per_rep(m[2], b[2], n),
                   'a' + j, per_rep(e[2], m[2], n));
        }

        printf("\n");
        fflush(stdout);
    }

    for (int j = 0; j < 2; j++) {
        disconnect(s[j].c, s[j].vol);
        free(s[j].tp);
        free(s[j].td);
        free(s[j].cp);
        free(s[j].cd);
    }

    free(dc);
    free(du);
    printf("## pair sessions closed\n");
}

/* -M: a lookup of navmark-<label>, which afp_pcap.py and afpd_log.py take
 * as the start of phase <label>; the name does not exist */
static void mark(CONN *c, uint16_t vol, const char *label, int round)
{
    char name[64];

    if (!Markers) {
        return;
    }

    snprintf(name, sizeof(name), "navmark-%s%d", label, round);
    (void)FPGetFileDirParams(c, vol, DIRDID_ROOT, name, 0, 1 << DIRPBIT_DID);
}

/*! @brief Walk the tree as the Finder does, Rounds times, timing each level */
static void navigate(CONN *c, uint16_t vol)
{
    for (int r = 0; r < Rounds; r++) {
        printf("## round %d: descend\n", r + 1);
        mark(c, vol, "down", r + 1);

        for (int k = 0; k <= Depth; k++) {
            display(c, vol, k, "down");
        }

        dump_request("after descent");
        printf("## round %d: ascend\n", r + 1);
        mark(c, vol, "up", r + 1);

        for (int k = Depth - 1; k >= 0; k--) {
            display(c, vol, k, "up");
        }

        dump_request("after ascent");
    }

    mark(c, vol, "end", 0);
}

/*! @brief Parse the options and run the chosen mode: create, navigate,
 *         benchmark or remove */
int main(int argc, char **argv)
{
    int cc;

    while ((cc = getopt(argc, argv, "h:p:q:u:w:s:D:W:R:r:A:B:dScnMPx")) != EOF) {
        switch (cc) {
        case 'B':
            BenchReps = (int)num_opt(optarg, 1, 1000000, "-B");
            break;

        case 'A':
            /* the cleartext login takes the plain FPLoginExt path */
            Uam = afptest_uam_uses_legacy_login(optarg) ? NULL : optarg;
            break;

        case 'M':
            Markers = true;
            break;

        case 'P':
            ByPath = true;
            break;

        case 'h':
            Server = optarg;
            break;

        case 'p':
            Port = (int)num_opt(optarg, 1, 65535, "-p");
            break;

        case 'q':
            Port2 = (int)num_opt(optarg, 1, 65535, "-q");
            break;

        case 'u':
            User = optarg;
            break;

        case 'w':
            Password = optarg;
            break;

        case 's':
            Vol = optarg;
            break;

        case 'D':
            Depth = (int)num_opt(optarg, 1, MAXD, "-D");
            break;

        case 'W':
            Width = (int)num_opt(optarg, 1, MAXW, "-W");
            break;

        case 'R':
            RsrcLen = (int)num_opt(optarg, 1, MAXR, "-R");
            break;

        case 'r':
            Rounds = (int)num_opt(optarg, 1, 1000000, "-r");
            break;

        case 'd':
            WriteDSStore = true;
            break;

        case 'S':
            SignalDumps = true;
            break;

        case 'c':
            CreateOnly = true;
            break;

        case 'n':
            NavigateOnly = true;
            break;

        case 'x':
            RemoveOnly = true;
            break;

        default:
            fprintf(stderr, "usage: afp_navtest -h host -p port -u user -w pass -s vol "
                            "[-A uam] [-D depth] [-W width] [-R rsrcbytes] [-r rounds] "
                            "[-B reps [-q port2]] [-d] [-S] [-M] [-P] [-c|-n|-x]\n");
            return 2;
        }
    }

    if (Port2 > 0 && (BenchReps <= 0 || !NavigateOnly)) {
        fprintf(stderr, "navtest: -q takes -B and -n\n");
        return 2;
    }

    if ((CreateOnly && (NavigateOnly || RemoveOnly))
            || (NavigateOnly && RemoveOnly)) {
        fprintf(stderr, "navtest: -c, -n and -x exclude one another\n");
        return 2;
    }

    Quiet = 1;

    for (int i = 0; i < (int)sizeof(Data); i++) {
        Data[i] = (char)('a' + i % 26);
    }

    /* -x removes the tree a -c made, -W folders wide at its top */
    if (RemoveOnly) {
        CONN *c = connect_login();
        uint16_t vol = FPOpenVol(c, Vol);

        if (vol == 0xffff) {
            die("FPOpenVol", 0);
        }

        for (int i = 0; i < Width; i++) {
            char name[64];
            char marker[96];
            snprintf(name, sizeof(name), "L1_d%02d", i);
            snprintf(marker, sizeof(marker), "%s/%s", name, MARKER);

            /* only a folder this tool made, and marked, goes */
            if (FPGetFileDirParams(c, vol, DIRDID_ROOT, marker,
                                   1 << FILPBIT_FNUM, 0) != 0) {
                fprintf(stderr, "navtest: %s has no %s marker; left alone\n",
                        name, MARKER);
                continue;
            }

            delete_directory_tree(c, vol, DIRDID_ROOT, name);
        }

        disconnect(c, vol);
        printf("## tree removed\n");
        return 0;
    }

    if (!NavigateOnly) {
        CONN *c = connect_login();
        uint16_t vol = FPOpenVol(c, Vol);

        if (vol == 0xffff) {
            die("FPOpenVol", 0);
        }

        create_tree(c, vol);
        disconnect(c, vol);
        printf("## creation session closed\n");
        fflush(stdout);
        sleep(1);
    }

    if (CreateOnly) {
        return 0;
    }

    if (Port2 > 0) {
        bench_pair();
        return 0;
    }

    {
        CONN *c = connect_session(&Session);
        uint16_t vol = FPOpenVol(c, Vol);

        if (vol == 0xffff) {
            die("FPOpenVol", 0);
        }

        if (NavigateOnly) {
            learn_tree(c, vol);
        }

        printf("## navigation session: path DIDs:");

        for (int L = 0; L <= Depth; L++) {
            printf(" %u", ntohl(folder_did[L]));
        }

        printf("\n");

        if (BenchReps > 0) {
            bench(c, vol);
        } else {
            navigate(c, vol);
        }

        disconnect(c, vol);
        printf("## navigation session closed\n");
    }

    return 0;
}
