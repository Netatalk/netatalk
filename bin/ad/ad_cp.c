/*
 * Copyright (c) 2010, Frank Lahm <franklahm@googlemail.com>
 * Copyright (c) 1988, 1993, 1994
 * The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * David Hitz of Auspex Systems Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Cp copies source files to target files.
 *
 * The global PATH_T structure "to" always contains the path to the
 * current target file.  Since fts(3) does not change directories,
 * this path can be either absolute or dot-relative.
 *
 * The basic algorithm is to initialize "to" and use fts(3) to traverse
 * the file hierarchy rooted in the argument list.  A trivial case is the
 * case of 'cp file1 file2'.  The more interesting case is the case of
 * 'cp file1 file2 ... fileN dir' where the hierarchy is traversed and the
 * path (relative to the root of the traversal) is appended to dir (stored
 * in "to") to form the final target path.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <atalk/ftw.h>
#include <atalk/adouble.h>
#include <atalk/vfs.h>
#include <atalk/util.h>
#include <atalk/unix.h>
#include <atalk/volume.h>
#include <atalk/volinfo.h>
#include <atalk/bstrlib.h>
#include <atalk/bstradd.h>
#include <atalk/queue.h>

#include "ad.h"

#define STRIP_TRAILING_SLASH(p) {                                   \
        while ((p).p_end > (p).p_path + 1 && (p).p_end[-1] == '/')  \
            *--(p).p_end = 0;                                       \
    }

static char emptystring[] = "";

PATH_T to = { to.p_path, emptystring, "" };

int fflag, iflag, lflag, nflag, pflag, vflag;
mode_t mask;
struct volinfo svolinfo, dvolinfo;
struct vol svolume, dvolume;
cnid_t did = 0; /* current dir CNID */

static enum op type;
static int Rflag;
volatile sig_atomic_t sigint;
static int badcp, rval;
static int ftw_options = FTW_MOUNT | FTW_PHYS | FTW_ACTIONRETVAL;
static q_t *cnidq;              /* CNID dir stack */

enum op { FILE_TO_FILE, FILE_TO_DIR, DIR_TO_DNE };

static char           *netatalk_dirs[] = {
    ".AppleDouble",
    ".AppleDB",
    ".AppleDesktop",
    NULL
};

static int copy(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf);
static void siginfo(int _U_);

/*
  Check for netatalk special folders e.g. ".AppleDB" or ".AppleDesktop"
  Returns pointer to name or NULL.
*/
static const char *check_netatalk_dirs(const char *name)
{
    int c;

    for (c=0; netatalk_dirs[c]; c++) {
        if ((strcmp(name, netatalk_dirs[c])) == 0)
            return netatalk_dirs[c];
    }
    return NULL;
}


static void usage_cp(void)
{
    printf(
        "Usage: ad cp [-R [-P]] [-pvf] <source_file> <target_file>\n"
        "Usage: ad cp [-R [-P]] [-pvfx] <source_file [source_file ...]> <target_directory>\n"
        );
    exit(EXIT_FAILURE);
}

static void upfunc(void)
{
    if (cnidq) {
        cnid_t *cnid = dequeue(cnidq);
        if (cnid) {
            did = *cnid;
            free(cnid);
        }
    }
}

int ad_cp(int argc, char *argv[])
{
    struct stat to_stat, tmp_stat;
    int r, ch, have_trailing_slash;
    char *target;
#if 0
    afpvol_t srcvol;
    afpvol_t dstvol;
#endif

    while ((ch = getopt(argc, argv, "Rafilnpvx")) != -1)
        switch (ch) {
        case 'R':
            Rflag = 1;
            break;
        case 'a':
            pflag = 1;
            Rflag = 1;
            break;
        case 'f':
            fflag = 1;
            iflag = nflag = 0;
            break;
        case 'i':
            iflag = 1;
            fflag = nflag = 0;
            break;
        case 'l':
            lflag = 1;
            break;
        case 'n':
            nflag = 1;
            fflag = iflag = 0;
            break;
        case 'p':
            pflag = 1;
            break;
        case 'v':
            vflag = 1;
            break;
        case 'x':
            ftw_options |= FTW_MOUNT;
            break;
        default:
            usage_cp();
            break;
        }
    argc -= optind;
    argv += optind;

    if (argc < 2)
        usage_cp();

    (void)signal(SIGINT, siginfo);

    cnid_init();

    /* Save the target base in "to". */
    target = argv[--argc];
    if ((strlcpy(to.p_path, target, PATH_MAX)) >= PATH_MAX)
        ERROR("%s: name too long", target);

    to.p_end = to.p_path + strlen(to.p_path);
    if (to.p_path == to.p_end) {
        *to.p_end++ = '.';
        *to.p_end = 0;
    }
    have_trailing_slash = (to.p_end[-1] == '/');
    if (have_trailing_slash)
        STRIP_TRAILING_SLASH(to);
    to.target_end = to.p_end;

    /* Set end of argument list */
    argv[argc] = NULL;

    /*
     * Cp has two distinct cases:
     *
     * cp [-R] source target
     * cp [-R] source1 ... sourceN directory
     *
     * In both cases, source can be either a file or a directory.
     *
     * In (1), the target becomes a copy of the source. That is, if the
     * source is a file, the target will be a file, and likewise for
     * directories.
     *
     * In (2), the real target is not directory, but "directory/source".
     */
    r = stat(to.p_path, &to_stat);
    if (r == -1 && errno != ENOENT)
        ERROR("%s", to.p_path);
    if (r == -1 || !S_ISDIR(to_stat.st_mode)) {
        /*
         * Case (1).  Target is not a directory.
         */
        if (argc > 1)
            ERROR("%s is not a directory", to.p_path);

        /*
         * Need to detect the case:
         *cp -R dir foo
         * Where dir is a directory and foo does not exist, where
         * we want pathname concatenations turned on but not for
         * the initial mkdir().
         */
        if (r == -1) {
            lstat(*argv, &tmp_stat);

            if (S_ISDIR(tmp_stat.st_mode) && Rflag)
                type = DIR_TO_DNE;
            else
                type = FILE_TO_FILE;
        } else
            type = FILE_TO_FILE;

        if (have_trailing_slash && type == FILE_TO_FILE) {
            if (r == -1)
                ERROR("directory %s does not exist", to.p_path);
            else
                ERROR("%s is not a directory", to.p_path);
        }
    } else
        /*
         * Case (2).  Target is a directory.
         */
        type = FILE_TO_DIR;

    /*
     * Keep an inverted copy of the umask, for use in correcting
     * permissions on created directories when not using -p.
     */
    mask = ~umask(0777);
    umask(~mask);

#if 0
    /* Inhereting perms in ad_mkdir etc requires this */
    ad_setfuid(0);
#endif

    /* Load .volinfo file for destination*/
    if (loadvolinfo(to.p_path, &dvolinfo) == 0) {
        if (vol_load_charsets(&dvolinfo) == -1)
            ERROR("Error loading charsets!");
        /* Sanity checks to ensure we can touch this volume */
        if (dvolinfo.v_vfs_ea != AFPVOL_EA_SYS)
            ERROR("Unsupported Extended Attributes option: %u", dvolinfo.v_vfs_ea);

        /* initialize sufficient struct vol and initialize CNID connection */
        dvolume.v_adouble = AD_VERSION2;
        dvolume.v_vfs_ea = AFPVOL_EA_SYS;
        initvol_vfs(&dvolume);
        int flags = 0;
        if ((dvolinfo.v_flags & AFPVOL_NODEV))
            flags |= CNID_FLAG_NODEV;

        if ((dvolume.v_cdb = cnid_open(dvolinfo.v_path,
                                       0000,
                                       "dbd",
                                       flags,
                                       dvolinfo.v_dbd_host,
                                       dvolinfo.v_dbd_port)) == NULL)
            ERROR("Cant initialize CNID database connection for %s", dvolinfo.v_path);

        /* setup a list for storing the CNID stack of dirs in destination path */
        if ((cnidq = queue_init()) == NULL)
            ERROR("Cant initialize CNID stack");
    }

    for (int i = 0; argv[i] != NULL; i++) { 
        /* Load .volinfo file for source */
        if (loadvolinfo(to.p_path, &svolinfo) == 0) {
            if (vol_load_charsets(&svolinfo) == -1)
                ERROR("Error loading charsets!");
            /* Sanity checks to ensure we can touch this volume */
            if (svolinfo.v_vfs_ea != AFPVOL_EA_SYS)
                ERROR("Unsupported Extended Attributes option: %u", svolinfo.v_vfs_ea);

            /* initialize sufficient struct vol and initialize CNID connection */
            svolume.v_adouble = AD_VERSION2;
            svolume.v_vfs_ea = AFPVOL_EA_SYS;
            initvol_vfs(&svolume);
            int flags = 0;
            if ((svolinfo.v_flags & AFPVOL_NODEV))
                flags |= CNID_FLAG_NODEV;

            if ((svolume.v_cdb = cnid_open(svolinfo.v_path,
                                           0000,
                                           "dbd",
                                           flags,
                                           svolinfo.v_dbd_host,
                                           svolinfo.v_dbd_port)) == NULL)
                ERROR("Cant initialize CNID database connection for %s", svolinfo.v_path);
        }

        if (nftw(argv[i], copy, upfunc, 20, ftw_options) == -1) {
            ERROR("%s: %s", argv[i], strerror(errno));
            exit(EXIT_FAILURE);
        }

        if (svolume.v_cdb)
            cnid_close(svolume.v_cdb);
        svolume.v_cdb = NULL;

    }
    return rval;
}

static int copy(const char *path,
                const struct stat *statp,
                int tflag,
                struct FTW *ftw)
{
    struct stat to_stat;
    int base = 0, dne;
    size_t nlen;
    const char *p;
    char *target_mid;

    const char *dir = strrchr(path, '/');
    if (dir == NULL)
        dir = path;
    else
        dir++;
    if (check_netatalk_dirs(dir) != NULL) {
        SLOG("Skipping Netatalk dir %s", path);
        return FTW_SKIP_SIBLINGS;
    }

    /*
     * If we are in case (2) above, we need to append the
     * source name to the target name.
     */
    if (type != FILE_TO_FILE) {
        /*
         * Need to remember the roots of traversals to create
         * correct pathnames.  If there's a directory being
         * copied to a non-existent directory, e.g.
         *     cp -R a/dir noexist
         * the resulting path name should be noexist/foo, not
         * noexist/dir/foo (where foo is a file in dir), which
         * is the case where the target exists.
         *
         * Also, check for "..".  This is for correct path
         * concatenation for paths ending in "..", e.g.
         *     cp -R .. /tmp
         * Paths ending in ".." are changed to ".".  This is
         * tricky, but seems the easiest way to fix the problem.
         *
         * XXX
         * Since the first level MUST be FTS_ROOTLEVEL, base
         * is always initialized.
         */
        if (ftw->level == 0) {
            if (type != DIR_TO_DNE) {
                base = ftw->base;

                if (strcmp(&path[base], "..") == 0)
                    base += 1;
            } else
                base = strlen(path);
        }

        p = &path[base];
        nlen = strlen(path) - base;
        target_mid = to.target_end;
        if (*p != '/' && target_mid[-1] != '/')
            *target_mid++ = '/';
        *target_mid = 0;
        if (target_mid - to.p_path + nlen >= PATH_MAX) {
            SLOG("%s%s: name too long (not copied)", to.p_path, p);
            badcp = rval = 1;
            return 0;
        }
        (void)strncat(target_mid, p, nlen);
        to.p_end = target_mid + nlen;
        *to.p_end = 0;
        STRIP_TRAILING_SLASH(to);
    }

    /* Not an error but need to remember it happened */
    if (stat(to.p_path, &to_stat) == -1)
        dne = 1;
    else {
        if (to_stat.st_dev == statp->st_dev &&
            to_stat.st_ino == statp->st_ino) {
            SLOG("%s and %s are identical (not copied).",
                to.p_path, path);
            badcp = rval = 1;
            if (S_ISDIR(statp->st_mode))
                /* without using glibc extension FTW_ACTIONRETVAL cant handle this */
                return -1;
        }
        if (!S_ISDIR(statp->st_mode) &&
            S_ISDIR(to_stat.st_mode)) {
            SLOG("cannot overwrite directory %s with "
                "non-directory %s",
                to.p_path, path);
                badcp = rval = 1;
                return 0;
        }
        dne = 0;
    }

    switch (statp->st_mode & S_IFMT) {
    case S_IFLNK:
        if (copy_link(ftw, path, statp, !dne))
            badcp = rval = 1;
        break;
    case S_IFDIR:
        if (!Rflag) {
            SLOG("%s is a directory", path);
            badcp = rval = 1;
            return -1;
        }
        /*
         * If the directory doesn't exist, create the new
         * one with the from file mode plus owner RWX bits,
         * modified by the umask.  Trade-off between being
         * able to write the directory (if from directory is
         * 555) and not causing a permissions race.  If the
         * umask blocks owner writes, we fail..
         */
        if (dne) {
            if (mkdir(to.p_path, statp->st_mode | S_IRWXU) < 0)
                ERROR("%s", to.p_path);
        } else if (!S_ISDIR(to_stat.st_mode)) {
            errno = ENOTDIR;
            ERROR("%s", to.p_path);
        }

        /* Create ad dir and copy ".Parent" */
        if (svolinfo.v_path && svolinfo.v_adouble == AD_VERSION2 &&
            dvolinfo.v_path && dvolinfo.v_adouble == AD_VERSION2) {
            /* Create ".AppleDouble" dir */
            mode_t omask = umask(0);
            bstring addir = bfromcstr(to.p_path);
            bcatcstr(addir, "/.AppleDouble");
            mkdir(cfrombstr(addir), 02777);

            /* copy ".Parent" file */
            bcatcstr(addir, "/.Parent");
            bstring sdir = bfromcstr(path);
            bcatcstr(sdir, "/.AppleDouble/.Parent");
            if (copy_file(-1, cfrombstr(sdir), cfrombstr(addir), 0666) != 0) {
                SLOG("Error copying %s -> %s", cfrombstr(sdir), cfrombstr(addir));
                badcp = rval = 1;
                break;
            }
            umask(omask);

            /* Get CNID of Parent and add new childir to CNID database */
            did = cnid_for_path(&dvolinfo, &dvolume, to.p_path);
        }

        if (pflag) {
            if (setfile(statp, -1))
                rval = 1;
#if 0
            if (preserve_dir_acls(statp, curr->fts_accpath, to.p_path) != 0)
                rval = 1;
#endif
        }
        break;

    case S_IFBLK:
    case S_IFCHR:
        SLOG("%s is a device file (not copied).", path);
        break;
    case S_IFSOCK:
        SLOG("%s is a socket (not copied).", path);
        break;
    case S_IFIFO:
        SLOG("%s is a FIFO (not copied).", path);
        break;
    default:
        if (ftw_copy_file(ftw, path, statp, dne))
            badcp = rval = 1;
        break;
    }
    if (vflag && !badcp)
        (void)printf("%s -> %s\n", path, to.p_path);

    return 0;
}

static void siginfo(int sig _U_)
{
    sigint = 1;
}
