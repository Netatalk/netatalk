/*
 * Copyright (c) 2010, Frank Lahm <franklahm@googlemail.com>
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

#include <sys/types.h>
#include <sys/wait.h>
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

static int fflg, iflg, nflg, vflg;

static afpvol_t svolume, dvolume;
static cnid_t did, pdid;
static volatile sig_atomic_t alarmed;
static char *adexecp;
static char           *netatalk_dirs[] = {
    ".AppleDouble",
    ".AppleDB",
    ".AppleDesktop",
    NULL
};

static int copy(const char *, const char *);
static int do_move(const char *, const char *);
static int fastcopy(const char *, const char *, struct stat *);
static void preserve_fd_acls(int source_fd, int dest_fd, const char *source_path,
                             const char *dest_path);
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

/*
  SIGNAL handling:
  catch SIGINT and SIGTERM which cause clean exit. Ignore anything else.
*/

static void sig_handler(int signo)
{
    alarmed = 1;
    return;
}

static void set_signal(void)
{
    struct sigaction sv;

    sv.sa_handler = sig_handler;
    sv.sa_flags = SA_RESTART;
    sigemptyset(&sv.sa_mask);
    if (sigaction(SIGTERM, &sv, NULL) < 0)
        ERROR("error in sigaction(SIGTERM): %s", strerror(errno));

    if (sigaction(SIGINT, &sv, NULL) < 0)
        ERROR("error in sigaction(SIGINT): %s", strerror(errno));

    memset(&sv, 0, sizeof(struct sigaction));
    sv.sa_handler = SIG_IGN;
    sigemptyset(&sv.sa_mask);

    if (sigaction(SIGABRT, &sv, NULL) < 0)
        ERROR("error in sigaction(SIGABRT): %s", strerror(errno));

    if (sigaction(SIGHUP, &sv, NULL) < 0)
        ERROR("error in sigaction(SIGHUP): %s", strerror(errno));

    if (sigaction(SIGQUIT, &sv, NULL) < 0)
        ERROR("error in sigaction(SIGQUIT): %s", strerror(errno));
}

static void usage_mv(void)
{
    printf(
        "Usage: ad mv [-f | -i | -n] [-v] source target\n"
        "       ad mv [-f | -i | -n] [-v] source ... directory\n"
        );
    exit(EXIT_FAILURE);
}

int ad_mv(int argc, char *argv[])
{
    size_t baselen, len;
    int rval;
    char *p, *endp;
    struct stat sb;
    int ch;
    char path[MAXPATHLEN];


    pdid = htonl(1);
    did = htonl(2);

    adexecp = argv[0];
    argc--;
    argv++;

    while ((ch = getopt(argc, argv, "finv")) != -1)
        switch (ch) {
        case 'i':
            iflg = 1;
            fflg = nflg = 0;
            break;
        case 'f':
            fflg = 1;
            iflg = nflg = 0;
            break;
        case 'n':
            nflg = 1;
            fflg = iflg = 0;
            break;
        case 'v':
            vflg = 1;
            break;
        default:
            usage_mv();
        }

    argc -= optind;
    argv += optind;

    if (argc < 2)
        usage_mv();

    set_signal();
    cnid_init();
    if (openvol(argv[argc - 1], &dvolume) != 0) {
        SLOG("Error opening CNID database for %s: ", argv[argc - 1]);
        return 1;
    }

    /*
     * If the stat on the target fails or the target isn't a directory,
     * try the move.  More than 2 arguments is an error in this case.
     */
    if (stat(argv[argc - 1], &sb) || !S_ISDIR(sb.st_mode)) {
        if (argc > 2)
            usage_mv();
        if (openvol(argv[0], &svolume) != 0) {
            SLOG("Error opening CNID database for %s: ", argv[0]);
            return 1;
        }
        rval = do_move(argv[0], argv[1]);
        closevol(&svolume);
        closevol(&dvolume);
    }

    /* It's a directory, move each file into it. */
    if (strlen(argv[argc - 1]) > sizeof(path) - 1)
        ERROR("%s: destination pathname too long", *argv);

    (void)strcpy(path, argv[argc - 1]);
    baselen = strlen(path);
    endp = &path[baselen];
    if (!baselen || *(endp - 1) != '/') {
        *endp++ = '/';
        ++baselen;
    }

    for (rval = 0; --argc; ++argv) {
        /*
         * Find the last component of the source pathname.  It
         * may have trailing slashes.
         */
        p = *argv + strlen(*argv);
        while (p != *argv && p[-1] == '/')
            --p;
        while (p != *argv && p[-1] != '/')
            --p;

        if ((baselen + (len = strlen(p))) >= PATH_MAX) {
            SLOG("%s: destination pathname too long", *argv);
            rval = 1;
        } else {
            memmove(endp, p, (size_t)len + 1);
            if (openvol(*argv, &svolume) != 0) {
                SLOG("Error opening CNID database for %s: ", argv[0]);
                rval = 1;
                goto exit;
            }
            if (do_move(*argv, path))
                rval = 1;
            closevol(&svolume);
        }
    }

exit:
    closevol(&dvolume);
    return rval;
}

static int do_move(const char *from, const char *to)
{
    struct stat sb;
    int ask, ch, first;

    /*
     * Check access.  If interactive and file exists, ask user if it
     * should be replaced.  Otherwise if file exists but isn't writable
     * make sure the user wants to clobber it.
     */
    if (!fflg && !access(to, F_OK)) {

        /* prompt only if source exist */
        if (lstat(from, &sb) == -1) {
            SLOG("%s: %s", from, strerror(errno));
            return (1);
        }

        ask = 0;
        if (nflg) {
            if (vflg)
                printf("%s not overwritten\n", to);
            return (0);
        } else if (iflg) {
            (void)fprintf(stderr, "overwrite %s? (y/n [n]) ", to);
            ask = 1;
        } else if (access(to, W_OK) && !stat(to, &sb)) {
            (void)fprintf(stderr, "override for %s? (y/n [n]) ", to);
            ask = 1;
        }
        if (ask) {
            first = ch = getchar();
            while (ch != '\n' && ch != EOF)
                ch = getchar();
            if (first != 'y' && first != 'Y') {
                (void)fprintf(stderr, "not overwritten\n");
                return (0);
            }
        }
    }

    if (!rename(from, to)) {
        if (vflg)
            printf("%s -> %s\n", from, to);
        return (0);
    }

    if (errno == EXDEV) {
        char path[MAXPATHLEN];

        /*
         * If the source is a symbolic link and is on another
         * filesystem, it can be recreated at the destination.
         */
        if (lstat(from, &sb) == -1) {
            SLOG("%s: %s", from, strerror(errno));
            return (1);
        }
        if (!S_ISLNK(sb.st_mode)) {
            /* Can't mv(1) a mount point. */
            if (realpath(from, path) == NULL) {
                SLOG("cannot resolve %s: %s: %s", from, path, strerror(errno));
                return (1);
            }
        }
    } else {
        SLOG("rename %s to %s: %s", from, to, strerror(errno));
        return (1);
    }

    /*
     * If rename fails because we're trying to cross devices, and
     * it's a regular file, do the copy internally; otherwise, use
     * cp and rm.
     */
    if (lstat(from, &sb)) {
        SLOG("%s: %s", from, strerror(errno));
        return (1);
    }
    return (S_ISREG(sb.st_mode) ?
            fastcopy(from, to, &sb) : copy(from, to));
}

static int fastcopy(const char *from, const char *to, struct stat *sbp)
{
    struct timeval tval[2];
    static u_int blen;
    static char *bp;
    mode_t oldmode;
    int nread, from_fd, to_fd;

    if ((from_fd = open(from, O_RDONLY, 0)) < 0) {
        SLOG("%s: %s", from, strerror(errno));
        return (1);
    }
    if (blen < sbp->st_blksize) {
        if (bp != NULL)
            free(bp);
        if ((bp = malloc((size_t)sbp->st_blksize)) == NULL) {
            blen = 0;
            SLOG("malloc failed");
            return (1);
        }
        blen = sbp->st_blksize;
    }
    while ((to_fd = open(to, O_CREAT | O_EXCL | O_TRUNC | O_WRONLY, 0)) < 0) {
        if (errno == EEXIST && unlink(to) == 0)
            continue;
        SLOG("%s: %s", to, strerror(errno));
        (void)close(from_fd);
        return (1);
    }
    while ((nread = read(from_fd, bp, (size_t)blen)) > 0)
        if (write(to_fd, bp, (size_t)nread) != nread) {
            SLOG("%s: %s", to, strerror(errno));
            goto err;
        }
    if (nread < 0) {
        SLOG("%s: %s", from, strerror(errno));
    err:
        if (unlink(to))
            SLOG("%s: remove", to, strerror(errno));
        (void)close(from_fd);
        (void)close(to_fd);
        return (1);
    }

    oldmode = sbp->st_mode & (S_IRWXU | S_IRWXG | S_IRWXO | S_ISUID | S_ISGID);
    if (fchown(to_fd, sbp->st_uid, sbp->st_gid)) {
        SLOG("%s: set owner/group (was: %lu/%lu)", to,
             (u_long)sbp->st_uid, (u_long)sbp->st_gid, strerror(errno));
        if (oldmode & S_ISUID) {
            SLOG("%s: owner/group changed; clearing suid (mode was 0%03o)",
                 to, oldmode);
            sbp->st_mode &= ~S_ISUID;
        }
    }
    if (fchmod(to_fd, sbp->st_mode))
        SLOG("%s: set mode (was: 0%03o): %s", to, oldmode, strerror(errno));
    /*
     * POSIX 1003.2c states that if _POSIX_ACL_EXTENDED is in effect
     * for dest_file, then its ACLs shall reflect the ACLs of the
     * source_file.
     */
    preserve_fd_acls(from_fd, to_fd, from, to);
    (void)close(from_fd);

    tval[0].tv_sec = sbp->st_atime;
    tval[1].tv_sec = sbp->st_mtime;
    tval[0].tv_usec = tval[1].tv_usec = 0;
    if (utimes(to, tval))
        SLOG("%s: set times: %s", to, strerror(errno));

    if (close(to_fd)) {
        SLOG("%s: %s", to, strerror(errno));
        return (1);
    }

    if (unlink(from)) {
        SLOG("%s: remove: %s", from, strerror(errno));
        return (1);
    }
    if (vflg)
        printf("%s -> %s\n", from, to);
    return 0;
}

static int copy(const char *from, const char *to)
{
    struct stat sb;
    int pid, status;

    if (lstat(to, &sb) == 0) {
        /* Destination path exists. */
        if (S_ISDIR(sb.st_mode)) {
            if (rmdir(to) != 0) {
                SLOG("rmdir %s: %s", to, strerror(errno));
                return (1);
            }
        } else {
            if (unlink(to) != 0) {
                SLOG("unlink %s: %s", to, strerror(errno));
                return (1);
            }
        }
    } else if (errno != ENOENT) {
        SLOG("%s: %s", to, strerror(errno));
        return (1);
    }

    /* Copy source to destination. */
    if (!(pid = fork())) {
        execl(adexecp, "cp", vflg ? "-Rpv" : "-Rp", "--", from, to, (char *)NULL);
        _exit(1);
    }
    if (waitpid(pid, &status, 0) == -1) {
        SLOG("%s %s %s: waitpid: %s", adexecp, from, to, strerror(errno));
        return (1);
    }
    if (!WIFEXITED(status)) {
        SLOG("%s %s %s: did not terminate normally", adexecp, from, to);
        return (1);
    }
    switch (WEXITSTATUS(status)) {
    case 0:
        break;
    default:
        SLOG("%s %s %s: terminated with %d (non-zero) status",
              adexecp, from, to, WEXITSTATUS(status));
        return (1);
    }

    /* Delete the source. */
    if (!(pid = fork())) {
        execl(adexecp, "rm", "-Rf", "--", from, (char *)NULL);
        _exit(1);
    }
    if (waitpid(pid, &status, 0) == -1) {
        SLOG("%s %s: waitpid: %s", adexecp, from, strerror(errno));
        return (1);
    }
    if (!WIFEXITED(status)) {
        SLOG("%s %s: did not terminate normally", adexecp, from);
        return (1);
    }
    switch (WEXITSTATUS(status)) {
    case 0:
        break;
    default:
        SLOG("%s %s: terminated with %d (non-zero) status",
              adexecp, from, WEXITSTATUS(status));
        return (1);
    }
    return 0;
}

static void
preserve_fd_acls(int source_fd,
                 int dest_fd,
                 const char *source_path,
                 const char *dest_path)
{
    ;
}
