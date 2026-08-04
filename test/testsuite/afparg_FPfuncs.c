#include <errno.h>
#include <limits.h>
#include <time.h>

#include "afpcmd.h"
#include "afphelper.h"
#include "testhelper.h"

/* Require argv[0..count-1] to be present; a missing argument is a usage
 * error (NOT TESTED), not a test failure. */
static int args_required(char **argv, int count, const char *usage)
{
    for (int i = 0; i < count; i++) {
        if (argv[i] == NULL || argv[i][0] == '\0') {
            fprintf(stdout, "missing argument(s), usage: %s\n", usage);
            test_nottested();
            return -1;
        }
    }

    return 0;
}

/* Parse an optional hold-seconds argument.  NULL/empty means hold until
 * SIGINT (returns -1); anything else must be a positive integer. */
static long parse_hold_seconds(const char *arg)
{
    char *end;
    long secs;

    if (arg == NULL || arg[0] == '\0') {
        return -1;
    }

    errno = 0;
    secs = strtol(arg, &end, 10);

    if (errno != 0 || *end != '\0' || secs <= 0 || secs > 86400) {
        fprintf(stdout, "invalid hold seconds \"%s\" (1-86400)\n", arg);
        return -2;
    }

    return secs;
}

/* Hold the open fork for `secs` seconds (or until SIGINT when secs < 0),
 * then close it cleanly.  A signal-killed client never sends FPLogout, so
 * the afpd session would park disconnected and keep the fork's locks. */
static void hold_and_close_fork(uint16_t fork, long secs)
{
    if (secs > 0) {
        sleep((unsigned int)secs);
    } else {
        pause();
    }

    FAIL(FPCloseFork(Conn, fork))
}

void FPCopyFile_arg(char **argv)
{
    uint16_t vol = VolID;
    fprintf(stdout, "======================\n");
    fprintf(stdout, "FPCopyFile with args:\n");
    fprintf(stdout, "source: \"%s\" -> dest: \"%s\"\n", argv[0], argv[1]);
    FAIL(FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, argv[0], "", argv[1]))
}

/* ----------- */
void FPResolveID_arg(char **argv)
{
    int argc = 0;
    unsigned int ret, ofs = 3 * sizeof(uint16_t);
    uint16_t bitmap = (1 << FILPBIT_PDINFO);
    uint32_t id;
    const DSI *dsi = &Conn->dsi;
    struct afp_filedir_parms filedir = { 0 };
    fprintf(stdout, "======================\n");
    fprintf(stdout, "FPResolveID with args:\n");
    id = atoi(argv[0]);
    fprintf(stdout, "Trying to resolve id %u\n", id);

    if (Conn->afp_version < 30) {
        test_skipped(T_AFP3);
        return;
    }

    if (!(get_vol_attrib(VolID) & VOLPBIT_ATTR_UTF8)) {
        test_skipped(T_UTF8);
        return;
    }

    if (!(get_vol_attrib(VolID) & VOLPBIT_ATTR_FILEID)) {
        test_skipped(T_ID);
        return;
    }

    if (FPResolveID(Conn, VolID, htonl(id), bitmap)) {
        test_failed();
        return;
    }

    filedir.isdir = 0;
    afp_filedir_unpack(Conn, &filedir, dsi->data + 2, bitmap, 0);
    fprintf(stdout, "Resolved ID %d to: '%s'\n", id, filedir.utf8_name);
}

static void handler()
{
    return;
}

/* Shared body of FPLockrw/FPLockw: open argv[1]'s d|r fork (argv[0]) with
 * `access`, hold for optional argv[2] seconds (default: until SIGINT),
 * close cleanly. */
static void lock_hold_common(char **argv, const char *what, int access)
{
    uint16_t vol = VolID;
    uint16_t fork;
    struct sigaction action;
    int toopen;
    long secs;

    if (args_required(argv, 2, "d | r file [seconds]") < 0) {
        return;
    }

    secs = parse_hold_seconds(argv[2]);

    if (secs == -2) {
        test_nottested();
        return;
    }

    if (argv[0][0] == 'd') {
        toopen = OPENFORK_DATA;
    } else {
        toopen = OPENFORK_RSCS;
    }

    fprintf(stdout, "======================\n");
    fprintf(stdout, "FPOpen with %s\n", what);
    fprintf(stdout, "source: \"%s\"\n", argv[1]);
    action.sa_handler = handler;
    sigemptyset(&action.sa_mask);

    if (sigaction(SIGINT, &action, NULL) < 0) {
        test_nottested();
        goto test_exit;
    }

    fork = FPOpenFork(Conn, vol, toopen, 0, DIRDID_ROOT, argv[1], access);

    if (!fork) {
        test_nottested();
        goto test_exit;
    }

    hold_and_close_fork(fork, secs);
test_exit:
    action.sa_handler = SIG_DFL;
    (void)sigaction(SIGINT, &action, NULL);
}

void FPLockrw_arg(char **argv)
{
    lock_hold_common(argv, "read/write lock",
                     OPENACC_RD | OPENACC_WR | OPENACC_DRD | OPENACC_DWR);
}

void FPLockw_arg(char **argv)
{
    lock_hold_common(argv, "write lock", OPENACC_RD | OPENACC_DWR);
}

void FPEnumerate_arg(char **argv)
{
    uint8_t buffer[DSI_DATASIZ];
    uint16_t vol = VolID;
    uint16_t d_bitmap;
    uint16_t f_bitmap;
    unsigned int ret;
    int dir;
    uint16_t tp;
    uint16_t i;
    const DSI *dsi = &Conn->dsi;
    const unsigned char *b;
    struct afp_filedir_parms filedir = { 0 };
    int *stack = NULL;
    int cnt = 0;
    int size = 1000;
    f_bitmap = (1 << FILPBIT_FNUM) | (1 << FILPBIT_ATTR) | (1 << FILPBIT_FINFO) |
               (1 << FILPBIT_CDATE) | (1 << FILPBIT_BDATE) | (1 << FILPBIT_MDATE) |
               (1 << FILPBIT_DFLEN) | (1 << FILPBIT_RFLEN);
    d_bitmap = (1 << DIRPBIT_ATTR) | (1 << DIRPBIT_FINFO) | (1 << DIRPBIT_OFFCNT) |
               (1 << DIRPBIT_CDATE) | (1 << DIRPBIT_BDATE) | (1 << DIRPBIT_MDATE) |
               (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) | (1 << DIRPBIT_ACCESS);

    if (Conn->afp_version >= 30) {
        f_bitmap |= (1 << FILPBIT_PDINFO) | (1 << FILPBIT_LNAME);
        d_bitmap |= (1 << FILPBIT_PDINFO) | (1 << FILPBIT_LNAME);
    } else {
        f_bitmap |= (1 << FILPBIT_LNAME);
        d_bitmap |= (1 << FILPBIT_LNAME);
    }

    if (!Quiet) {
        fprintf(stdout, "start time %ld\n", time(NULL));
    }

    if (argv[0] == NULL) {
        dir = get_did(Conn, vol, DIRDID_ROOT, "");
    } else {
        dir = get_did(Conn, vol, DIRDID_ROOT, argv[0]);
    }

    if (!dir) {
        test_nottested();
        goto fin;
    }

    if (!(stack = calloc(size, sizeof(int)))) {
        test_nottested();
        goto fin;
    }

    stack[cnt] = dir;
    cnt++;

    while (cnt) {
        cnt--;
        dir = stack[cnt];
        i = 1;

        if (FPGetFileDirParams(Conn, vol, dir, "", 0, d_bitmap)) {
            test_nottested();
            goto fin;
        }

        /* FIXME: FPEnumerate* uses dsi_data_receive. See afphelper.c:delete_directory_tree() */

        while (!(ret = FPEnumerateFull(Conn, vol, i, 150, 8000,  dir, "", f_bitmap,
                                       d_bitmap))) {
            /* FPResolveID will trash dsi->data */
            memcpy(buffer, dsi->data, sizeof(buffer));
            memcpy(&tp, buffer + 4, sizeof(tp));
            tp = ntohs(tp);
            i += tp;
            b = buffer + 6;

            for (int j = 1; j <= tp; j++, b += b[0]) {
                if (b[1]) {
                    filedir.isdir = 1;
                    afp_filedir_unpack(Conn, &filedir, b + 2, 0, d_bitmap);

                    if (cnt > size) {
                        size += 1000;

                        if (!(stack = realloc(stack, size * sizeof(int)))) {
                            test_nottested();
                            goto fin;
                        }
                    }

                    stack[cnt] = filedir.did;
                    cnt++;
                } else {
                    filedir.isdir = 0;
                    afp_filedir_unpack(Conn, &filedir, b + 2, f_bitmap, 0);
                }

                if (!Quiet) {
                    fprintf(stdout, "0x%08x %s%s\n", ntohl(filedir.did),
                            (Conn->afp_version >= 30) ? filedir.utf8_name : filedir.lname,
                            filedir.isdir ? "/" : "");
                } else {
                    fprintf(stdout, "%s%s\n",
                            (Conn->afp_version >= 30) ? filedir.utf8_name : filedir.lname,
                            filedir.isdir ? "/" : "");
                }

                if (!filedir.isdir && FPResolveID(Conn, vol, filedir.did, f_bitmap) && !Quiet) {
                    fprintf(stdout, " Can't resolve ID!");
                }
            }
        }

        if (ret != ntohl(AFPERR_NOOBJ)) {
            test_nottested();
            goto fin;
        }
    }

    if (!Quiet) {
        fprintf(stdout, "end time %ld\n", time(NULL));
    }

    FPEnumerateFull(Conn, vol, 1, 150, 8000,  DIRDID_ROOT, "", f_bitmap, d_bitmap);
fin:
    free(stack);
}

/* Write argv[1] (string content) to file argv[0], creating it if needed.
 * Scriptable one-shot for cross-protocol interop checks. */
void FPWrite_arg(char **argv)
{
    uint16_t vol = VolID;
    uint16_t fork;
    unsigned int ret;
    size_t len;

    if (args_required(argv, 2, "file content") < 0) {
        return;
    }

    len = strlen(argv[1]);
    fprintf(stdout, "======================\n");
    fprintf(stdout, "FPWrite with args:\n");
    fprintf(stdout, "file: \"%s\" (%zu bytes)\n", argv[0], len);
    ret = FPCreateFile(Conn, vol, 0, DIRDID_ROOT, argv[0]);

    if (ret && ret != ntohl(AFPERR_EXIST)) {
        test_nottested();
        return;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, DIRDID_ROOT, argv[0],
                      OPENACC_WR | OPENACC_RD);

    if (!fork) {
        test_nottested();
        return;
    }

    /* truncate, then write the payload from offset 0 */
    if (FPSetForkParam(Conn, fork, 1 << FILPBIT_DFLEN, 0)) {
        test_failed();
        FPCloseFork(Conn, fork);
        return;
    }

    FAIL(FPWrite(Conn, fork, 0, len, argv[1], 0))
    FAIL(FPCloseFork(Conn, fork))
}

/* The tool prints the fork inline as text; cap reads at 1 MiB, plenty for
 * interop fixtures and far below FPRead's int-typed size parameter. */
#define FPREAD_ARG_MAX (1024 * 1024)

/* Read the data fork of file argv[0] and print it as DATA:<content>. */
void FPRead_arg(char **argv)
{
    uint16_t vol = VolID;
    uint16_t fork;
    uint32_t flen;
    char *buf;
    DSI *dsi = &Conn->dsi;

    if (args_required(argv, 1, "file") < 0) {
        return;
    }

    fprintf(stdout, "======================\n");
    fprintf(stdout, "FPRead with args:\n");
    fprintf(stdout, "file: \"%s\"\n", argv[0]);
    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, DIRDID_ROOT, argv[0],
                      OPENACC_RD);

    if (!fork) {
        test_nottested();
        return;
    }

    /* get_forklen() parses a FPGetForkParam reply (bitmap + params); the
     * FPOpenFork reply has a refnum between them, so ask again. */
    if (FPGetForkParam(Conn, fork, 1 << FILPBIT_DFLEN)) {
        test_failed();
        FPCloseFork(Conn, fork);
        return;
    }

    flen = get_forklen(dsi, OPENFORK_DATA);

    if (flen > FPREAD_ARG_MAX) {
        fprintf(stdout, "fork is %u bytes, tool caps at %d\n", flen,
                FPREAD_ARG_MAX);
        test_nottested();
        FPCloseFork(Conn, fork);
        return;
    }

    buf = calloc(1, (size_t)flen + 1);

    if (buf == NULL) {
        test_nottested();
        FPCloseFork(Conn, fork);
        return;
    }

    if (flen > 0 && FPRead(Conn, fork, 0, (int)flen, buf)) {
        test_failed();
        free(buf);
        FPCloseFork(Conn, fork);
        return;
    }

    fprintf(stdout, "DATA:%s\n", buf);
    free(buf);
    FAIL(FPCloseFork(Conn, fork))
}

/* Set extended attribute argv[1] = argv[2] on file argv[0]. */
void FPSetEA_arg(char **argv)
{
    uint16_t vol = VolID;

    if (args_required(argv, 3, "file attribute value") < 0) {
        return;
    }

    fprintf(stdout, "======================\n");
    fprintf(stdout, "FPSetExtAttr with args:\n");
    fprintf(stdout, "file: \"%s\", %s=%s\n", argv[0], argv[1], argv[2]);
    FAIL(FPSetExtAttr(Conn, vol, DIRDID_ROOT, 0, argv[0], argv[1], argv[2]))
}

/* Get extended attribute argv[1] of file argv[0]; prints EA:<value>. */
void FPGetEA_arg(char **argv)
{
    uint16_t vol = VolID;
    uint32_t attrlen;
    char attrval[1024];
    const DSI *dsi = &Conn->dsi;

    if (args_required(argv, 2, "file attribute") < 0) {
        return;
    }

    fprintf(stdout, "======================\n");
    fprintf(stdout, "FPGetExtAttr with args:\n");
    fprintf(stdout, "file: \"%s\", attribute: %s\n", argv[0], argv[1]);

    /* maxsize 0 = size query: the server refuses oversized replies with
     * AFPERR_PARAM instead of truncating, so check before fetching. */
    if (FPGetExtAttr(Conn, vol, DIRDID_ROOT, 0, 0, argv[0], argv[1])) {
        test_failed();
        return;
    }

    memcpy(&attrlen, dsi->data + 2, 4);
    attrlen = ntohl(attrlen);

    if (attrlen >= sizeof(attrval)) {
        fprintf(stdout, "attribute is %u bytes, tool caps at %zu\n",
                attrlen, sizeof(attrval) - 1);
        test_nottested();
        return;
    }

    if (FPGetExtAttr(Conn, vol, DIRDID_ROOT, 0, sizeof(attrval), argv[0],
                     argv[1])) {
        test_failed();
        return;
    }

    memcpy(&attrlen, dsi->data + 2, 4);
    attrlen = ntohl(attrlen);

    if (attrlen >= sizeof(attrval)) {
        test_failed();
        return;
    }

    memcpy(attrval, dsi->data + 6, attrlen);
    attrval[attrlen] = '\0';
    fprintf(stdout, "EA:%s\n", attrval);
}

/* Set (argv[1] = "on") or clear (argv[1] = "off") DeleteInhibit on
 * argv[0].  FPSetFileParams with the ATTR bitmap consumes only the attr
 * word, which SETCLR semantics make self-contained (set vs clear the
 * named bits) - no read-modify-write needed. */
void FPSetInhibit_arg(char **argv)
{
    uint16_t vol = VolID;
    struct afp_filedir_parms filedir = { 0 };
    uint16_t bitmap = (1 << FILPBIT_ATTR);

    if (args_required(argv, 2, "file on | off") < 0) {
        return;
    }

    if (strcmp(argv[1], "on") != 0 && strcmp(argv[1], "off") != 0) {
        fprintf(stdout, "second argument must be on or off\n");
        test_nottested();
        return;
    }

    fprintf(stdout, "======================\n");
    fprintf(stdout, "FPSetInhibit with args:\n");
    fprintf(stdout, "file: \"%s\", mode: %s\n", argv[0], argv[1]);
    filedir.isdir = 0;
    filedir.attr = strcmp(argv[1], "on") == 0
                   ? ATTRBIT_NODELETE | ATTRBIT_SETCLR
                   : ATTRBIT_NODELETE;
    FAIL(FPSetFileParams(Conn, vol, DIRDID_ROOT, argv[0], bitmap, &filedir))
}

/* Hold an AFP byte-range lock (bytes 0..7) on argv[1]'s d|r fork
 * (argv[0]) for argv[2] seconds, then unlock and close cleanly. */
void FPByteLockHold_arg(char **argv)
{
    uint16_t vol = VolID;
    uint16_t fork;
    int toopen;
    long secs;

    if (args_required(argv, 3, "d | r file seconds") < 0) {
        return;
    }

    secs = parse_hold_seconds(argv[2]);

    if (secs < 0) {
        test_nottested();
        return;
    }

    if (argv[0][0] == 'd') {
        toopen = OPENFORK_DATA;
    } else {
        toopen = OPENFORK_RSCS;
    }

    fprintf(stdout, "======================\n");
    fprintf(stdout, "FPByteLockHold with args:\n");
    fprintf(stdout, "file: \"%s\", hold: %lds\n", argv[1], secs);
    fork = FPOpenFork(Conn, vol, toopen, 0, DIRDID_ROOT, argv[1],
                      OPENACC_RD | OPENACC_WR);

    if (!fork) {
        test_nottested();
        return;
    }

    if (FPByteLock(Conn, fork, 0, 0 /* lock */, 0, 8)) {
        test_failed();
        FPCloseFork(Conn, fork);
        return;
    }

    sleep((unsigned int)secs);
    FAIL(FPByteLock(Conn, fork, 0, 1 /* unlock */, 0, 8))
    FAIL(FPCloseFork(Conn, fork))
}
