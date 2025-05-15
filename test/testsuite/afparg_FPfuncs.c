#include "specs.h"
#include <time.h>

void FPCopyFile_arg(char **argv) {
    uint16_t vol = VolID;
    fprintf(stdout, "======================\n");
    fprintf(stdout, "FPCopyFile with args:\n");
    fprintf(stdout, "source: \"%s\" -> dest: \"%s\"\n", argv[0], argv[1]);
    FAIL(FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, argv[0], "", argv[1]))
}

/* ----------- */
void FPResolveID_arg(char **argv) {
    int argc = 0;
    unsigned int ret, ofs = 3 * sizeof(uint16_t);
    uint16_t bitmap = (1 << FILPBIT_PDINFO);
    uint32_t id;
    DSI *dsi;
    struct afp_filedir_parms filedir;
    dsi = &Conn->dsi;
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
    afp_filedir_unpack(&filedir, dsi->data + 2, bitmap, 0);
    fprintf(stdout, "Resolved ID %d to: '%s'\n", id, filedir.utf8_name);
}

static void handler() {
    return;
}

void FPLockrw_arg(char **argv) {
    uint16_t vol = VolID;
    int fork;
    struct sigaction action;
    int toopen;

    if (argv[0][0] == 'd') {
        toopen = OPENFORK_DATA;
    } else {
        toopen = OPENFORK_RSCS;
    }

    fprintf(stdout, "======================\n");
    fprintf(stdout, "FPOpen with read/write lock\n");
    fprintf(stdout, "source: \"%s\"\n", argv[1]);
    action.sa_handler = handler;
    sigemptyset(&action.sa_mask);

    if (sigaction(SIGINT, &action, NULL) < 0) {
        test_nottested();
        goto test_exit;
    }

    fork = FPOpenFork(Conn, vol, toopen, 0, DIRDID_ROOT, argv[1],
                      OPENACC_RD | OPENACC_WR | OPENACC_DRD | OPENACC_DWR);

    if (!fork) {
        test_nottested();
        goto fin;
    }

    pause();
    FAIL(FPCloseFork(Conn, fork))
fin:
test_exit:
    action.sa_handler = SIG_DFL;
    (void)sigaction(SIGINT, &action, NULL);
}

void FPLockw_arg(char **argv) {
    uint16_t vol = VolID;
    int fork;
    struct sigaction action;
    int toopen;

    if (argv[0][0] == 'd') {
        toopen = OPENFORK_DATA;
    } else {
        toopen = OPENFORK_RSCS;
    }

    fprintf(stdout, "======================\n");
    fprintf(stdout, "FPOpen with write lock\n");
    fprintf(stdout, "source: \"%s\"\n", argv[1]);
    action.sa_handler = handler;
    sigemptyset(&action.sa_mask);

    if (sigaction(SIGINT, &action, NULL) < 0) {
        test_nottested();
        goto test_exit;
    }

    fork = FPOpenFork(Conn, vol, toopen, 0, DIRDID_ROOT, argv[1],
                      OPENACC_RD | OPENACC_DWR);

    if (!fork) {
        test_nottested();
        goto fin;
    }

    pause();
    FAIL(FPCloseFork(Conn, fork))
fin:
test_exit:
    action.sa_handler = SIG_DFL;
    (void)sigaction(SIGINT, &action, NULL);
}

void FPEnumerate_arg(char **argv) {
    uint8_t buffer[DSI_DATASIZ];
    uint16_t vol = VolID;
    uint16_t d_bitmap;
    uint16_t f_bitmap;
    unsigned int ret;
    int dir;
    uint16_t tp;
    uint16_t i;
    const DSI *dsi;
    unsigned char *b;
    struct afp_filedir_parms filedir;
    int *stack = NULL;
    int cnt = 0;
    int size = 1000;
    dsi = &Conn->dsi;
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
                    afp_filedir_unpack(&filedir, b + 2, 0, d_bitmap);

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
                    afp_filedir_unpack(&filedir, b + 2, f_bitmap, 0);
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
