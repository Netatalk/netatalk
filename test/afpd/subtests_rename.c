/*
  Unit tests for the FPMoveAndRename PATH_MAX preflight in etc/afpd/filedir.c.

  The preflight answers one question before any rename happens: would the
  longest path below the source directory still fit MAXPATHLEN once that
  directory is renamed into the working directory? These tests build real
  directory trees and check that answer, so a regression in the walk or in the
  length arithmetic fails here instead of in a hostile filesystem state.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <atalk/directory.h>
#include <atalk/globals.h>

#include "directory.h"
#include "filedir.h"
#include "subtests.h"

/* A directory whose deepest path is prefix + depth*(component_len+1) bytes. */
static int make_tree(const char *base, size_t depth, size_t component_len,
                     char *deepest, size_t deepest_len)
{
    char path[PATH_MAX];
    char name[NAME_MAX + 1];
    size_t i;
    size_t j;

    if (mkdir(base, 0700) != 0 && errno != EEXIST) {
        return -1;
    }

    if (snprintf(path, sizeof(path), "%s", base) >= (int)sizeof(path)) {
        return -1;
    }

    if (component_len == 0 || component_len > NAME_MAX) {
        return -1;
    }

    for (i = 1; i <= depth; i++) {
        size_t cur = strlen(path);

        if (cur + 1 + component_len >= sizeof(path)) {
            return -1;
        }

        for (j = 0; j < component_len; j++) {
            name[j] = (char)('a' + (char)((i + j) % 26));
        }

        name[component_len] = '\0';
        path[cur] = '/';
        memcpy(path + cur + 1, name, component_len + 1);

        if (mkdir(path, 0700) != 0 && errno != EEXIST) {
            return -1;
        }
    }

    if (deepest != NULL
            && snprintf(deepest, deepest_len, "%s", path) >= (int)deepest_len) {
        return -1;
    }

    return 0;
}

/* Recursive unlink of a tree made by make_tree, without shelling out. */
static int rm_tree(const char *dir)
{
    DIR *dp;
    struct dirent *de;
    struct stat st;
    char child[PATH_MAX];

    if ((dp = opendir(dir)) == NULL) {
        return 0;
    }

    while ((de = readdir(dp)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) {
            continue;
        }

        if (snprintf(child, sizeof(child), "%s/%s", dir, de->d_name)
                >= (int)sizeof(child)) {
            continue;
        }

        if (lstat(child, &st) == 0 && S_ISDIR(st.st_mode)) {
            rm_tree(child);
        } else {
            unlink(child);
        }
    }

    closedir(dp);
    rmdir(dir);
    return 0;
}

/* A directory holding one file: the file is the longest descendant. */
static int make_file(const char *dir, const char *name)
{
    char path[PATH_MAX];
    FILE *fp;

    if (snprintf(path, sizeof(path), "%s/%s", dir, name) >= (int)sizeof(path)) {
        return -1;
    }

    if ((fp = fopen(path, "w")) == NULL) {
        return -1;
    }

    fclose(fp);
    return 0;
}

/* Longest path strictly below an open directory descriptor, at most budget. */
static int walk(const struct vol *vol, const char *path, size_t budget,
                size_t *longest)
{
    int fd = open(path, O_RDONLY | O_DIRECTORY);
    int ret;

    if (fd < 0) {
        return -1;
    }

    ret = longest_descendant_pathlen(vol, fd, budget, longest);
    close(fd);
    return ret;
}

int test012_rename_preflight_walk_lengths(struct vol *vol)
{
    char base[PATH_MAX];
    char deepest[PATH_MAX];
    const char *tmp = getenv("TMPDIR");
    size_t longest = 0;
    size_t expected;
    int rc = 1;

    if (tmp == NULL) {
        tmp = "/tmp";
    }

    if (snprintf(base, sizeof(base), "%s/nlrename011.XXXXXX", tmp)
            >= (int)sizeof(base)) {
        return 2;
    }

    if (mkdtemp(base) == NULL) {
        return 3;
    }

    /* 1. An empty directory has nothing below it, so the answer is 0. */
    if (walk(vol, base, MAXPATHLEN, &longest) != 0) {
        rc = 4;
        goto out;
    }

    if (longest != 0) {
        rc = 5;
        goto out;
    }

    /* 2. A nested chain of depth 3 and 1-char components: everything below
     *    base is base + "/" + 3 components, i.e. the deepest dir, minus the
     *    base path itself and its slash. */
    if (make_tree(base, 3, 1, deepest, sizeof(deepest)) != 0) {
        rc = 6;
        goto out;
    }

    if (walk(vol, base, MAXPATHLEN, &longest) != 0) {
        rc = 7;
        goto out;
    }

    expected = strlen(deepest) - strlen(base);

    if (longest != expected) {
        rc = 8;
        goto out;
    }

    /* 3. A short file directly in base is a descendant too, and being
     *    shorter than the deepest directory it must not shrink the answer. */
    if (make_file(base, "zz") != 0) {
        rc = 9;
        goto out;
    }

    if (walk(vol, base, MAXPATHLEN, &longest) != 0) {
        rc = 10;
        goto out;
    }

    if (longest != expected) {
        rc = 11;
        goto out;
    }

    /* 4. A budget below the real answer stops early and reports a value the
     *    caller reads as "over": the walk must not report a short count. */
    if (walk(vol, base, 1, &longest) != 0) {
        rc = 12;
        goto out;
    }

    if (longest <= 1) {
        rc = 13;
        goto out;
    }

    /* 5. A file whose name is longer than the budget is still counted as
     *    over-budget, not skipped. */
    if (walk(vol, base, expected, &longest) != 0) {
        rc = 14;
        goto out;
    }

    if (longest < expected) {
        rc = 15;
        goto out;
    }

    rc = 0;
out:
    rm_tree(base);
    return rc;
}

int test013_rename_preflight_decision(struct vol *vol)
{
    char base[PATH_MAX];
    char src_dir[PATH_MAX];
    char deepest[PATH_MAX];
    char *tmp = getenv("TMPDIR");
    char longname[PATH_MAX];
    struct dir src;
    struct dir cwd;
    bstring cwdpath = NULL;
    bstring srcname = NULL;
    int fd = -1;
    int rc = 1;
    (void)vol;

    if (tmp == NULL) {
        tmp = "/tmp";
    }

    if (snprintf(base, sizeof(base), "%s/nlrename012.XXXXXX", tmp)
            >= (int)sizeof(base)) {
        return 2;
    }

    if (mkdtemp(base) == NULL) {
        return 3;
    }

    /* The source directory "s" holding a 3-deep chain of 1-char names. */
    if (snprintf(src_dir, sizeof(src_dir), "%s/s", base)
            >= (int)sizeof(src_dir)) {
        rc = 4;
        goto out;
    }

    if (make_tree(src_dir, 3, 1, deepest, sizeof(deepest)) != 0) {
        rc = 5;
        goto out;
    }

    memset(&src, 0, sizeof(src));
    memset(&cwd, 0, sizeof(cwd));
    cwdpath = bfromcstr(base);
    srcname = bfromcstr("s");

    if (cwdpath == NULL || srcname == NULL) {
        rc = 6;
        goto out;
    }

    {
        /* curdir is a global; the preflight reads it for the destination. */
        struct dir *saved_curdir = curdir;
        curdir = &cwd;
        cwd.d_fullpath = cwdpath;
        src.d_fullpath = bfromcstr(src_dir);

        if (src.d_fullpath == NULL) {
            curdir = saved_curdir;
            rc = 7;
            goto out;
        }

        /* 1. A short destination name on a shallow tree fits. "s/a/a/a"
         *    is a few bytes, so nothing overflows. */
        if (rename_would_overflow_path(vol, -1, src_dir, &src, "s", 1) != 0) {
            rc = 8;
            curdir = saved_curdir;
            goto out;
        }

        /* 2. A file is never walked, so even a huge name is "no" here —
         *    the rename path itself rejects an oversized leaf name. */
        memset(longname, 'x', sizeof(longname) - 1);
        longname[sizeof(longname) - 1] = '\0';

        if (rename_would_overflow_path(vol, -1, src_dir, &src, longname,
                                       0) != 0) {
            rc = 9;
            curdir = saved_curdir;
            goto out;
        }

        /* 3. A directory destination name long enough that the new full
         *    path alone busts MAXPATHLEN must overflow. */
        if (rename_would_overflow_path(vol, -1, src_dir, &src, longname,
                                       1) != 1) {
            rc = 10;
            curdir = saved_curdir;
            goto out;
        }

        /* 4. Exact boundary. The kernel counts the NUL, so a deepest path of
         *    exactly MAXPATHLEN - 1 bytes fits and one byte more does not.
         *    namelen is the destination component length. */
        {
            int sfd;
            size_t below = 0;
            size_t rootlen = strlen(base) + 1;
            size_t namelen;

            if ((sfd = open(src_dir, O_RDONLY | O_DIRECTORY)) < 0
                    || longest_descendant_pathlen(vol, sfd, MAXPATHLEN,
                                                  &below) != 0) {
                rc = 11;
                curdir = saved_curdir;
                goto out;
            }

            close(sfd);

            /* deepest = root + namelen + below must stay < MAXPATHLEN. */
            if (rootlen + below >= MAXPATHLEN - 2) {
                rc = 12; /* tree does not leave room to test the edge */
                curdir = saved_curdir;
                goto out;
            }

            namelen = MAXPATHLEN - 1 - rootlen - below;

            if (namelen == 0 || namelen + 2 >= sizeof(longname)) {
                rc = 13;
                curdir = saved_curdir;
                goto out;
            }

            memset(longname, 'y', namelen);
            longname[namelen] = '\0';

            if (rename_would_overflow_path(vol, -1, src_dir, &src, longname,
                                           1) != 0) {
                rc = 14;
                curdir = saved_curdir;
                goto out;
            }

            longname[namelen] = 'y';
            longname[namelen + 1] = '\0';

            if (rename_would_overflow_path(vol, -1, src_dir, &src, longname,
                                           1) != 1) {
                rc = 15;
                curdir = saved_curdir;
                goto out;
            }
        }
        /* 5. A shortening rename on an over-limit tree must still be allowed,
         *    so the tree stays repairable over AFP. */
        {
            char short_src[PATH_MAX];
            struct dir deep;
            bstring deeppath;
            char *rel = deepest + strlen(base) + 1;

            if (snprintf(short_src, sizeof(short_src), "%s/%s", base, rel)
                    >= (int)sizeof(short_src)) {
                rc = 16;
                curdir = saved_curdir;
                goto out;
            }

            memset(&deep, 0, sizeof(deep));
            deeppath = bfromcstr(deepest);

            if (deeppath == NULL) {
                rc = 17;
                curdir = saved_curdir;
                goto out;
            }

            deep.d_fullpath = deeppath;

            /* Renaming the deep directory to a shorter name cannot lengthen
             * anything below it. */
            if (rename_would_overflow_path(vol, -1, deepest, &deep, "z",
                                           1) != 0) {
                rc = 18;
                bdestroy(deeppath);
                curdir = saved_curdir;
                goto out;
            }

            bdestroy(deeppath);
        }

        /* 6. The directory's own descriptor as the walk root, which is how
         *    afp_moveandrename hands it over. */
        if ((fd = open(src_dir, O_RDONLY | O_DIRECTORY)) < 0) {
            rc = 19;
            curdir = saved_curdir;
            goto out;
        }

        if (rename_would_overflow_path(vol, fd, ".", &src, "s", 1) != 0) {
            rc = 20;
            curdir = saved_curdir;
            goto out;
        }

        memset(longname, 'x', sizeof(longname) - 1);
        longname[sizeof(longname) - 1] = '\0';

        if (rename_would_overflow_path(vol, fd, ".", &src, longname, 1) != 1) {
            rc = 21;
            curdir = saved_curdir;
            goto out;
        }

        close(fd);
        fd = -1;
        /* 7. dirfd -1 with a relative name while the cwd is the parent,
         *    which is the FPRename layout: open(".") in afp_rename(). */
        {
            int prev = open(".", O_RDONLY);

            if (prev < 0 || chdir(base) != 0) {
                rc = 22;

                if (prev >= 0) {
                    close(prev);
                }

                curdir = saved_curdir;
                goto out;
            }

            if (rename_would_overflow_path(vol, -1, "s", &src, "s", 1) != 0) {
                rc = 23;
                fchdir(prev);
                close(prev);
                curdir = saved_curdir;
                goto out;
            }

            if (rename_would_overflow_path(vol, -1, "s", &src, longname,
                                           1) != 1) {
                rc = 24;
                fchdir(prev);
                close(prev);
                curdir = saved_curdir;
                goto out;
            }

            if (fchdir(prev) != 0) {
                rc = 25;
            }

            close(prev);
        }
        bdestroy(src.d_fullpath);
        curdir = saved_curdir;
    }

    if (rc == 1) {
        rc = 0;
    }

out:

    if (fd >= 0) {
        close(fd);
    }

    bdestroy(cwdpath);
    bdestroy(srcname);
    rm_tree(base);
    return rc;
}
