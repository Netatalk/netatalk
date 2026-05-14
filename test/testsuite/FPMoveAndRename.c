/* ----------------------------------------------
*/
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include <atalk/compat.h>

#include "afpcmd.h"
#include "afphelper.h"
#include "testhelper.h"

#define TESTSUITE_NAME_MAX 255

/* ------------------------- */
STATIC void test43()
{
    char *name = "t43 dir";
    char *name1 = "t43 subdir";
    uint16_t vol = VolID;
    int  dir = 0, dir1 = 0, dir2 = 0;
    ENTER_TEST
    dir  = FPCreateDir(Conn, vol, DIRDID_ROOT, name);

    if (!dir) {
        test_nottested();
        goto test_exit;
    }

    dir1  = FPCreateDir(Conn, vol, DIRDID_ROOT, name1);

    if (!dir1) {
        test_nottested();
        goto fin;
    }

    dir2  = FPCreateDir(Conn, vol, dir1, name1);

    if (!dir2) {
        test_nottested();
        goto fin;
    }

    if (FPMoveAndRename(Conn, vol, dir1, dir, name1, name1)) {
        test_failed();
        goto fin;
    }

    if (FPGetFileDirParams(Conn, vol, dir, name1,
                           0
                           ,
                           (1 << DIRPBIT_ATTR) | (1 << DIRPBIT_FINFO) |
                           (1 << DIRPBIT_CDATE) | (1 << DIRPBIT_BDATE) | (1 << DIRPBIT_MDATE) |
                           (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
                           (1 << DIRPBIT_ACCESS)
                          )
       ) {
        test_failed();
        goto fin;
    }

    if (!FPGetFileDirParams(Conn, vol, dir1, name1,
                            0
                            ,
                            (1 << DIRPBIT_ATTR) | (1 << DIRPBIT_FINFO) |
                            (1 << DIRPBIT_CDATE) | (1 << DIRPBIT_BDATE) | (1 << DIRPBIT_MDATE) |
                            (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
                            (1 << DIRPBIT_ACCESS)
                           )
       ) {
        test_failed();
        goto fin;
    }

fin:
    FAIL(dir2 && FPDelete(Conn, vol, dir2, ""))
    FAIL(dir1 && FPDelete(Conn, vol, dir1, ""))
    FAIL(dir && FPDelete(Conn, vol, dir, ""))
test_exit:
    exit_test("FPMoveAndRename:test43: move and rename folders");
}

/* -------------------------- */
STATIC void test73()
{
    int fork;
    int dir;
    int dir1;
    int dir2;
    uint16_t bitmap = 0;
    char *name  = "t73 Move and rename";
    char *name1 = "t73 Move and rename dest";
    char *name2 = "t73 dir";
    uint16_t vol = VolID;
    int ret;
    ENTER_TEST

    if (Mac) {
        test_skipped(T_MAC);
        goto test_exit;
    }

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name2))) {
        test_nottested();
        goto test_exit;
    }

    if (FPDelete(Conn, vol, DIRDID_ROOT, name2)) {
        test_nottested();
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    /* cname unchdirable */
    if (Conn2) {
        if (!(no_access_folder(vol, DIRDID_ROOT, "t73 700"))) {
            FPDelete(Conn, vol, dir, name);
            test_failed();
            goto test_exit;
        }

        ret = FPMoveAndRename(Conn, vol, DIRDID_ROOT, dir, "t73 700/essay", name1);

        if (not_valid(ret, /* MAC */AFPERR_NOOBJ, AFPERR_ACCESS)) {
            test_failed();
        }

        delete_folder(vol, DIRDID_ROOT, "t73 700");
    }

    /* bad ddid */
    FAIL(ntohl(AFPERR_NOOBJ) != FPMoveAndRename(Conn, vol, DIRDID_ROOT, dir, name,
            name1))
    FAIL(!(dir1 = FPCreateDir(Conn, vol, DIRDID_ROOT, name2)))
    FAIL(FPMoveAndRename(Conn, vol, DIRDID_ROOT, dir1, name, ""))
    FAIL(ntohl(AFPERR_NOOBJ) != FPDelete(Conn, vol, DIRDID_ROOT, name))
    FAIL(FPMoveAndRename(Conn, vol, dir1, DIRDID_ROOT, name, ""))
    /* dirty but well */
    FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name1))
    FAIL(ntohl(AFPERR_EXIST) !=
         FPMoveAndRename(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name, name1))
    FAIL(ntohl(AFPERR_EXIST) != FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name))
    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                      OPENACC_WR | OPENACC_RD);

    if (!fork) {
        test_failed();
    }

    if (fork) {
        FAIL(ntohl(AFPERR_EXIST) != FPMoveAndRename(Conn, vol, DIRDID_ROOT, DIRDID_ROOT,
                name1, name))
        FAIL(FPCloseFork(Conn, fork))
    }

    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name1))
    FAIL(!(dir2 = FPCreateDir(Conn, vol, DIRDID_ROOT, name)))
    FAIL(!FPCreateDir(Conn, vol, dir2, name1))
    FAIL(FPMoveAndRename(Conn, vol, DIRDID_ROOT, dir1,  name, ""))
    FAIL(FPDelete(Conn, vol, dir2, name1))
    FAIL(FPDelete(Conn, vol, dir1, name))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name2))
test_exit:
    exit_test("FPMoveAndRename:test73: Move and rename");
}

/* -------------------------- */
STATIC void test77()
{
    int fork;
    int dir;
    char *name = "t77 Move open fork other dir";
    char *name1 = "t77 dir";
    uint16_t vol = VolID;
    ENTER_TEST

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name1))) {
        test_nottested();
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto fin;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, DIRDID_ROOT, name,
                      OPENACC_RD | OPENACC_WR |  OPENACC_DWR);

    if (!fork) {
        test_failed();
        goto fin;
    }

    FAIL(FPMoveAndRename(Conn, vol, DIRDID_ROOT, dir, name, ""))
    FAIL(FPCloseFork(Conn, fork))
    FAIL(FPDelete(Conn, vol, dir, name))
    FAIL(ntohl(AFPERR_NOOBJ) != FPDelete(Conn, vol, DIRDID_ROOT, name))
fin:
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name1))
test_exit:
    exit_test("FPMoveAndRename:test77: Move an open fork in a different folder");
}

/* ------------------------------ */
STATIC void test123()
{
    int  dir;
    int  dir1 = 0, dir2 = 0, dir3 = 0, dir4 = 0, dir5 = 0;
    char *name = "t123 dir1";
    char *name1 = "t123 dir1_1";
    char *name2 = "t123 dir1_2";
    char *name3 = "t123 dir1_3";
    char *dest  = "t123 dest";
    char *dest1  = "t123 dest_1";
    uint16_t vol = VolID;
    ENTER_TEST

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name))) {
        test_nottested();
        goto fin;
    }

    if (!(dir1 = FPCreateDir(Conn, vol, dir, name1))) {
        test_nottested();
        goto fin;
    }

    if (!(dir2 = FPCreateDir(Conn, vol, dir, name2))) {
        test_nottested();
        goto fin;
    }

    if (!(dir3 = FPCreateDir(Conn, vol, dir, name3))) {
        test_nottested();
        goto fin;
    }

    if (!(dir4 = FPCreateDir(Conn, vol, DIRDID_ROOT, dest))) {
        test_nottested();
        goto fin;
    }

    if (!(dir5 = FPCreateDir(Conn, vol, dir4, dest1))) {
        test_nottested();
        goto fin;
    }

    FAIL(ntohl(AFPERR_CANTMOVE) != FPMoveAndRename(Conn, vol, DIRDID_ROOT, dir,
            name, ""))
    FAIL(ntohl(AFPERR_CANTMOVE) != FPMoveAndRename(Conn, vol, DIRDID_ROOT, dir,
            name, "new"))
    FAIL(FPMoveAndRename(Conn, vol, dir2, dir4, "", ""))
fin:
    FAIL(dir5 && FPDelete(Conn, vol, dir5, ""))
    FAIL(dir3 && FPDelete(Conn, vol, dir3, ""))
    FAIL(dir2 && FPDelete(Conn, vol, dir2, ""))
    FAIL(dir1 && FPDelete(Conn, vol, dir1, ""))
    FAIL(dir4 && FPDelete(Conn, vol, dir4, ""))
    FAIL(dir && FPDelete(Conn, vol, dir, ""))
    exit_test("FPMoveAndRename:test123: Move And Rename dir with sibling");
}

/* -------------------------- */
STATIC void test138()
{
    int  dir;
    char *name = "t138 file";
    char *name1 = "t138 dir";
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir = { 0 };
    uint16_t bitmap = (1 << DIRPBIT_ACCESS);
    uint16_t vol = VolID;
    const DSI *dsi = &Conn->dsi;
    ENTER_TEST

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name1))) {
        test_nottested();
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto fin;
    }

    FAIL(FPMoveAndRename(Conn, vol, DIRDID_ROOT, dir, name, ""))
    FAIL(FPMoveAndRename(Conn, vol, dir, DIRDID_ROOT, name, ""))

    if (FPGetFileDirParams(Conn, vol, dir, "", 0, bitmap)) {
        test_failed();
    } else {
        filedir.isdir = 1;
        afp_filedir_unpack(Conn, &filedir, dsi->data + ofs, 0, bitmap);
        filedir.access[0] = 0;
        filedir.access[1] = 3;
        filedir.access[2] = 3;
        filedir.access[3] = 3;
        FAIL(FPSetDirParms(Conn, vol, dir, "", bitmap, &filedir))
    }

    FAIL(ntohl(AFPERR_ACCESS) != FPMoveAndRename(Conn, vol, DIRDID_ROOT, dir, name,
            ""))
    filedir.access[1] = 7;
    filedir.access[2] = 7;
    filedir.access[3] = 7;
    FAIL(FPSetDirParms(Conn, vol, dir, "", bitmap, &filedir))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
fin:
    FAIL(FPDelete(Conn, vol, dir, ""))
test_exit:
    exit_test("FPMoveAndRename:test138: Move And Rename");
}

/* ------------------------- */
STATIC void test322()
{
    char *name = "t322 dir";
    char *name1 = "t322 file";
    char *name2 = "t322 file1";
    int dir;
    uint16_t vol = VolID;
    int id;
    int id1;
    ENTER_TEST

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name))) {
        test_nottested();
        goto test_exit;
    }

    FAIL(FPCreateFile(Conn, vol, 0, dir, name1))
    id = get_fid(Conn, vol, dir, name1);
    FAIL(FPMoveAndRename(Conn, vol, dir, dir, name1, name2))
    id1 = get_fid(Conn, vol, dir, name2);

    if (id != id1) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED id are not the same %d %d\n", ntohl(id), ntohl(id1));
        }

        test_failed();
    }

    FAIL(FPDelete(Conn, vol, dir, name2))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPMoveAndRename:test322: file renamed, cnid not updated");
}

/* ------------------------- */
STATIC void test378()
{
    char *name =  "t378 name";
    char *name1 = "t378 Name";
    uint16_t vol = VolID;
    int ret;
    ENTER_TEST

    /* AFP 2.x technically doesn't support case sensitive file system */
    if (Conn->afp_version < 30) {
        test_skipped(T_AFP3);
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_failed();
        goto test_exit;
    }

    if ((ret = FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name1))
            && ret != htonl(AFPERR_EXIST)) {
        test_failed();
        goto fin;
    }

    if (ret == htonl(AFPERR_EXIST)) {
        FAIL(FPMoveAndRename(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name, name1))
    } else {
        FAIL(htonl(AFPERR_EXIST) != FPMoveAndRename(Conn, vol, DIRDID_ROOT, DIRDID_ROOT,
                name, name1))
    }

    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name1))
fin:
    FPDelete(Conn, vol, DIRDID_ROOT, name);
test_exit:
    exit_test("FPMoveAndRename:test378: dest file exist but diff only by case, is this one OK");
}

#define DEEP_TREE_TARGET_COMPONENT_LEN 200
#define DEEP_TREE_DEFAULT_VOLUME_PREFIX_LEN 26
#define DEEP_TREE_LEAF_NAME "testfile"

struct test460_tree_plan {
    int depth;
    size_t component_len;
};

static size_t test460_volume_prefix_len(void)
{
    size_t len;

    if (!Path || Path[0] == '\0') {
        return DEEP_TREE_DEFAULT_VOLUME_PREFIX_LEN;
    }

    len = strnlen(Path, MAXPATHLEN);

    if (len == MAXPATHLEN) {
        return MAXPATHLEN;
    }

    return len + (Path[len - 1] == '/' ? 0 : 1);
}

static int test460_component_len(const char *name, size_t *len)
{
    if (!name || !len) {
        return -1;
    }

    *len = strnlen(name, TESTSUITE_NAME_MAX + 1);

    if (*len > TESTSUITE_NAME_MAX) {
        return -1;
    }

    return 0;
}

static int test460_tree_plan(const char *toplevel, const char *dst_renamed,
                             const char *leaf_name,
                             struct test460_tree_plan *plan)
{
    size_t max_path = MAXPATHLEN - 1;
    size_t prefix_len = test460_volume_prefix_len();
    size_t toplevel_len;
    size_t dst_renamed_len;
    size_t rename_delta;
    size_t leaf_name_len;
    size_t leaf_len;
    size_t fixed_len;
    size_t max_depth;
    size_t best_score = (size_t) -1;
    memset(plan, 0, sizeof(*plan));

    if (test460_component_len(toplevel, &toplevel_len) < 0
            || test460_component_len(dst_renamed, &dst_renamed_len) < 0
            || test460_component_len(leaf_name, &leaf_name_len) < 0) {
        return -1;
    }

    if (dst_renamed_len <= toplevel_len) {
        return -1;
    }

    rename_delta = dst_renamed_len - toplevel_len;
    leaf_len = leaf_name_len + 1; /* slash + leaf */

    if (prefix_len > max_path || toplevel_len > max_path - prefix_len
            || leaf_len > max_path - prefix_len - toplevel_len) {
        return -1;
    }

    fixed_len = prefix_len + toplevel_len + leaf_len;
    max_depth = (max_path - fixed_len) / 2; /* slash + at least one char */

    for (int depth = 1; (size_t)depth <= max_depth; depth++) {
        for (size_t component_len = 1; component_len <= TESTSUITE_NAME_MAX;
                component_len++) {
            size_t full_len = fixed_len + (size_t)depth * (component_len + 1);
            size_t score;

            if (full_len > max_path || full_len + rename_delta <= max_path) {
                continue;
            }

            score = component_len > DEEP_TREE_TARGET_COMPONENT_LEN
                    ? component_len - DEEP_TREE_TARGET_COMPONENT_LEN
                    : DEEP_TREE_TARGET_COMPONENT_LEN - component_len;

            if (score < best_score
                    || (score == best_score && depth > plan->depth)) {
                plan->depth = depth;
                plan->component_len = component_len;
                best_score = score;
            }
        }
    }

    return plan->depth > 0 ? 0 : -1;
}

/* ------------------------- */
STATIC void test460()
{
    uint16_t vol = VolID;
    int *dirs = NULL;
    struct test460_tree_plan plan;
    int ret;
    char *toplevel    = "t_cdov";         /* 6-char root dir */
    char *dst_renamed =
        "t_cdov_renamed";
    char  sub_name[TESTSUITE_NAME_MAX + 1];
    ENTER_TEST

    if (test460_tree_plan(toplevel, dst_renamed, DEEP_TREE_LEAF_NAME,
                          &plan) < 0) {
        test_nottested();
        goto test_exit;
    }

    dirs = calloc((size_t)plan.depth + 1, sizeof(*dirs));

    if (!dirs) {
        test_nottested();
        goto test_exit;
    }

    /*
     * Build a tree whose leaf still fits in this platform's MAXPATHLEN,
     * including the local volume path when known, but whose leaf would exceed
     * that limit after renaming the top-level directory.
     */
    dirs[0] = FPCreateDir(Conn, vol, DIRDID_ROOT, toplevel);

    if (!dirs[0]) {
        test_nottested();
        goto test_exit;
    }

    for (int i = 1; i <= plan.depth; i++) {
        memset(sub_name, 'a' + ((i - 1) % 26), plan.component_len);
        sub_name[plan.component_len] = '\0';
        dirs[i] = FPCreateDir(Conn, vol, dirs[i - 1], sub_name);

        if (!dirs[i]) {
            test_nottested();
            goto cleanup;
        }
    }

    if (FPCreateFile(Conn, vol, 0, dirs[plan.depth], DEEP_TREE_LEAF_NAME)) {
        test_nottested();
        goto cleanup;
    }

    ret = FPMoveAndRename(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, toplevel,
                          dst_renamed);

    if (FPGetSrvrParms(Conn)) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED: server unreachable after FPMoveAndRename\n");
        }

        test_failed();
        goto test_exit;
    }

    if (ret == AFP_OK) {
        FAIL(delete_directory_tree(Conn, vol, DIRDID_ROOT, dst_renamed))
        goto test_exit;
    }

cleanup:

    if (dirs && dirs[0]) {
        delete_directory_tree(Conn, vol, DIRDID_ROOT, toplevel);
    }

test_exit:
    free(dirs);
    exit_test("FPMoveAndRename:test460: deeply nested rename with path overflow");
}

/* ----------- */
void FPMoveAndRename_test()
{
    ENTER_TESTSET
    test43();
    test73();
    test77();
    test138();
    test322();
    test378();
    test460();
}
