/* 
 * Copyright (c) 2009 Frank Lahm <franklahm@gmail.com>
 * Copyright (c) 1991, 1993, 1994
 * The Regents of the University of California.  All rights reserved.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <sys/acl.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>

#include <atalk/cnid.h>
#include <atalk/volinfo.h>
#include "ad.h"

#define cp_pct(x, y)((y == 0) ? 0 : (int)(100.0 * (x) / (y)))

/* Memory strategy threshold, in pages: if physmem is larger then this, use a 
 * large buffer */
#define PHYSPAGES_THRESHOLD (32*1024)

/* Maximum buffer size in bytes - do not allow it to grow larger than this */
#define BUFSIZE_MAX (2*1024*1024)

/* Small (default) buffer size in bytes. It's inefficient for this to be
 * smaller than MAXPHYS */
#define MAXPHYS (64 * 1024)
#define BUFSIZE_SMALL (MAXPHYS)

int log_verbose;             /* Logging flag */

void _log(enum logtype lt, char *fmt, ...)
{
    int len;
    static char logbuffer[1024];
    va_list args;

    if ( (lt == STD) || (log_verbose == 1)) {
        va_start(args, fmt);
        len = vsnprintf(logbuffer, 1023, fmt, args);
        va_end(args);
        logbuffer[1023] = 0;

        printf("%s\n", logbuffer);
    }
}

int newvol(const char *path, afpvol_t *vol)
{
    //    char *pathdup;
    
    memset(vol, 0, sizeof(afpvol_t));

    //    pathdup = strdup(path);
    //    vol->dirname = strdup(dirname(pathdup));
    //    free(pathdup);
    
    //    pathdup = strdup(path);
    //    vol->basename = strdup(basename(pathdup));
    //    free(pathdup);

    loadvolinfo((char *)path, &vol->volinfo);

    return 0;
}

void freevol(afpvol_t *vol)
{
#if 0
    if (vol->dirname) {
        free(vol->dirname);
        vol->dirname = NULL;
    }
    if (vol->basename) {
        free(vol->basename);
        vol->basename = NULL;
    }
#endif
}

/*
  Taken form afpd/desktop.c
*/
char *utompath(const struct volinfo *volinfo, char *upath)
{
    static char  mpath[ MAXPATHLEN + 2]; /* for convert_charset dest_len parameter +2 */
    char         *m, *u;
    uint16_t     flags = CONV_IGNORE | CONV_UNESCAPEHEX;
    size_t       outlen;

    if (!upath)
        return NULL;

    m = mpath;
    u = upath;
    outlen = strlen(upath);

    if ((volinfo->v_casefold & AFPVOL_UTOMUPPER))
        flags |= CONV_TOUPPER;
    else if ((volinfo->v_casefold & AFPVOL_UTOMLOWER))
        flags |= CONV_TOLOWER;

    if ((volinfo->v_flags & AFPVOL_EILSEQ)) {
        flags |= CONV__EILSEQ;
    }

    /* convert charsets */
    if ((size_t)-1 == ( outlen = convert_charset(volinfo->v_volcharset,
                                                 CH_UTF8_MAC,
                                                 volinfo->v_maccharset,
                                                 u, outlen, mpath, MAXPATHLEN, &flags)) ) {
        SLOG("Conversion from %s to %s for %s failed.",
            volinfo->v_volcodepage, volinfo->v_maccodepage, u);
        return NULL;
    }

    return(m);
}

/*!
 * Resolves CNID of a given paths parent directory
 *
 * path might be:
 * (a) relative:
 *     "dir/subdir" with cwd: "/afp_volume/topdir"
 * (b) absolute:
 *     "/afp_volume/dir/subdir"
 *
 * 1) in case a) concatenate both paths
 * 2) strip last element
 * 3) strip volume root
 * 4) start recursive CNID search with
 *    a) DID:2, "topdir"
 *    b) DID:2, "dir"
 * 5) ...until we have the CNID for
 *    a) "/afp_volume/topdir/dir"
 *    b) "/afp_volume/dir" (no recursion required)
 */
cnid_t get_parent_cnid_for_path(const struct volinfo *vi,
                                const struct vol *vol,
                                const char *path)
{

    return 0;
}

int ftw_copy_file(const struct FTW *entp,
              const char *spath,
              const struct stat *sp,
              int dne)
{
    static char *buf = NULL;
    static size_t bufsize;
    ssize_t wcount;
    size_t wresid;
    off_t wtotal;
    int ch, checkch, from_fd = 0, rcount, rval, to_fd = 0;
    char *bufp;
    char *p;

    if ((from_fd = open(spath, O_RDONLY, 0)) == -1) {
        SLOG("%s: %s", spath, strerror(errno));
        return (1);
    }

    /*
     * If the file exists and we're interactive, verify with the user.
     * If the file DNE, set the mode to be the from file, minus setuid
     * bits, modified by the umask; arguably wrong, but it makes copying
     * executables work right and it's been that way forever.  (The
     * other choice is 666 or'ed with the execute bits on the from file
     * modified by the umask.)
     */
    if (!dne) {
#define YESNO "(y/n [n]) "
        if (nflag) {
            if (vflag)
                printf("%s not overwritten\n", to.p_path);
            (void)close(from_fd);
            return (0);
        } else if (iflag) {
            (void)fprintf(stderr, "overwrite %s? %s", 
                          to.p_path, YESNO);
            checkch = ch = getchar();
            while (ch != '\n' && ch != EOF)
                ch = getchar();
            if (checkch != 'y' && checkch != 'Y') {
                (void)close(from_fd);
                (void)fprintf(stderr, "not overwritten\n");
                return (1);
            }
        }
        
        if (fflag) {
            /* remove existing destination file name, 
             * create a new file  */
            (void)unlink(to.p_path);
            if (!lflag)
                to_fd = open(to.p_path, O_WRONLY | O_TRUNC | O_CREAT,
                             sp->st_mode & ~(S_ISUID | S_ISGID));
        } else {
            if (!lflag)
                /* overwrite existing destination file name */
                to_fd = open(to.p_path, O_WRONLY | O_TRUNC, 0);
        }
    } else {
        if (!lflag)
            to_fd = open(to.p_path, O_WRONLY | O_TRUNC | O_CREAT,
                         sp->st_mode & ~(S_ISUID | S_ISGID));
    }
    
    if (to_fd == -1) {
        SLOG("%s: %s", to.p_path, strerror(errno));
        (void)close(from_fd);
        return (1);
    }

    rval = 0;

    if (!lflag) {
        /*
         * Mmap and write if less than 8M (the limit is so we don't totally
         * trash memory on big files.  This is really a minor hack, but it
         * wins some CPU back.
         * Some filesystems, such as smbnetfs, don't support mmap,
         * so this is a best-effort attempt.
         */

        if (S_ISREG(sp->st_mode) && sp->st_size > 0 &&
            sp->st_size <= 8 * 1024 * 1024 &&
            (p = mmap(NULL, (size_t)sp->st_size, PROT_READ,
                      MAP_SHARED, from_fd, (off_t)0)) != MAP_FAILED) {
            wtotal = 0;
            for (bufp = p, wresid = sp->st_size; ;
                 bufp += wcount, wresid -= (size_t)wcount) {
                wcount = write(to_fd, bufp, wresid);
                if (wcount <= 0)
                    break;
                wtotal += wcount;
                if (wcount >= (ssize_t)wresid)
                    break;
            }
            if (wcount != (ssize_t)wresid) {
                SLOG("%s: %s", to.p_path, strerror(errno));
                rval = 1;
            }
            /* Some systems don't unmap on close(2). */
            if (munmap(p, sp->st_size) < 0) {
                SLOG("%s: %s", spath, strerror(errno));
                rval = 1;
            }
        } else {
            if (buf == NULL) {
                /*
                 * Note that buf and bufsize are static. If
                 * malloc() fails, it will fail at the start
                 * and not copy only some files. 
                 */ 
                if (sysconf(_SC_PHYS_PAGES) > 
                    PHYSPAGES_THRESHOLD)
                    bufsize = MIN(BUFSIZE_MAX, MAXPHYS * 8);
                else
                    bufsize = BUFSIZE_SMALL;
                buf = malloc(bufsize);
                if (buf == NULL)
                    ERROR("Not enough memory");

            }
            wtotal = 0;
            while ((rcount = read(from_fd, buf, bufsize)) > 0) {
                for (bufp = buf, wresid = rcount; ;
                     bufp += wcount, wresid -= wcount) {
                    wcount = write(to_fd, bufp, wresid);
                    if (wcount <= 0)
                        break;
                    wtotal += wcount;
                    if (wcount >= (ssize_t)wresid)
                        break;
                }
                if (wcount != (ssize_t)wresid) {
                    SLOG("%s: %s", to.p_path, strerror(errno));
                    rval = 1;
                    break;
                }
            }
            if (rcount < 0) {
                SLOG("%s: %s", spath, strerror(errno));
                rval = 1;
            }
        }
    } else {
        if (link(spath, to.p_path)) {
            SLOG("%s", to.p_path);
            rval = 1;
        }
    }
    
    /*
     * Don't remove the target even after an error.  The target might
     * not be a regular file, or its attributes might be important,
     * or its contents might be irreplaceable.  It would only be safe
     * to remove it if we created it and its length is 0.
     */

    if (!lflag) {
        if (pflag && setfile(sp, to_fd))
            rval = 1;
        if (pflag && preserve_fd_acls(from_fd, to_fd) != 0)
            rval = 1;
        if (close(to_fd)) {
            SLOG("%s: %s", to.p_path, strerror(errno));
            rval = 1;
        }
    }

    (void)close(from_fd);

    return (rval);
}

int copy_link(const struct FTW *p,
              const char *spath,
              const struct stat *sstp,
              int exists)
{
    int len;
    char llink[PATH_MAX];

    if ((len = readlink(spath, llink, sizeof(llink) - 1)) == -1) {
        SLOG("readlink: %s: %s", spath, strerror(errno));
        return (1);
    }
    llink[len] = '\0';
    if (exists && unlink(to.p_path)) {
        SLOG("unlink: %s: %s", to.p_path, strerror(errno));
        return (1);
    }
    if (symlink(llink, to.p_path)) {
        SLOG("symlink: %s: %s", llink, strerror(errno));
        return (1);
    }
    return (pflag ? setfile(sstp, -1) : 0);
}

int setfile(const struct stat *fs, int fd)
{
    static struct timeval tv[2];
    struct stat ts;
    int rval, gotstat, islink, fdval;
    mode_t mode;

    rval = 0;
    fdval = fd != -1;
    islink = !fdval && S_ISLNK(fs->st_mode);
    mode = fs->st_mode & (S_ISUID | S_ISGID | S_ISVTX | S_IRWXU | S_IRWXG | S_IRWXO);

    TIMESPEC_TO_TIMEVAL(&tv[0], &fs->st_atim);
    TIMESPEC_TO_TIMEVAL(&tv[1], &fs->st_mtim);
    if (islink ? lutimes(to.p_path, tv) : utimes(to.p_path, tv)) {
        SLOG("%sutimes: %s", islink ? "l" : "", to.p_path);
        rval = 1;
    }
    if (fdval ? fstat(fd, &ts) :
        (islink ? lstat(to.p_path, &ts) : stat(to.p_path, &ts)))
        gotstat = 0;
    else {
        gotstat = 1;
        ts.st_mode &= S_ISUID | S_ISGID | S_ISVTX |
            S_IRWXU | S_IRWXG | S_IRWXO;
    }
    /*
     * Changing the ownership probably won't succeed, unless we're root
     * or POSIX_CHOWN_RESTRICTED is not set.  Set uid/gid before setting
     * the mode; current BSD behavior is to remove all setuid bits on
     * chown.  If chown fails, lose setuid/setgid bits.
     */
    if (!gotstat || fs->st_uid != ts.st_uid || fs->st_gid != ts.st_gid)
        if (fdval ? fchown(fd, fs->st_uid, fs->st_gid) :
            (islink ? lchown(to.p_path, fs->st_uid, fs->st_gid) :
             chown(to.p_path, fs->st_uid, fs->st_gid))) {
            if (errno != EPERM) {
                SLOG("chown: %s: %s", to.p_path, strerror(errno));
                rval = 1;
            }
            mode &= ~(S_ISUID | S_ISGID);
        }

    if (!gotstat || mode != ts.st_mode)
        if (fdval ? fchmod(fd, mode) : chmod(to.p_path, mode)) {
            SLOG("chmod: %s: %s", to.p_path, strerror(errno));
            rval = 1;
        }

#ifdef HAVE_ST_FLAGS
    if (!gotstat || fs->st_flags != ts.st_flags)
        if (fdval ?
            fchflags(fd, fs->st_flags) :
            (islink ? lchflags(to.p_path, fs->st_flags) :
             chflags(to.p_path, fs->st_flags))) {
            SLOG("chflags: %s: %s", to.p_path, strerror(errno));
            rval = 1;
        }
#endif

    return (rval);
}

int preserve_fd_acls(int source_fd, int dest_fd)
{
#if 0
    acl_t acl;
    acl_type_t acl_type;
    int acl_supported = 0, ret, trivial;

    ret = fpathconf(source_fd, _PC_ACL_NFS4);
    if (ret > 0 ) {
        acl_supported = 1;
        acl_type = ACL_TYPE_NFS4;
    } else if (ret < 0 && errno != EINVAL) {
        warn("fpathconf(..., _PC_ACL_NFS4) failed for %s", to.p_path);
        return (1);
    }
    if (acl_supported == 0) {
        ret = fpathconf(source_fd, _PC_ACL_EXTENDED);
        if (ret > 0 ) {
            acl_supported = 1;
            acl_type = ACL_TYPE_ACCESS;
        } else if (ret < 0 && errno != EINVAL) {
            warn("fpathconf(..., _PC_ACL_EXTENDED) failed for %s",
                 to.p_path);
            return (1);
        }
    }
    if (acl_supported == 0)
        return (0);

    acl = acl_get_fd_np(source_fd, acl_type);
    if (acl == NULL) {
        warn("failed to get acl entries while setting %s", to.p_path);
        return (1);
    }
    if (acl_is_trivial_np(acl, &trivial)) {
        warn("acl_is_trivial() failed for %s", to.p_path);
        acl_free(acl);
        return (1);
    }
    if (trivial) {
        acl_free(acl);
        return (0);
    }
    if (acl_set_fd_np(dest_fd, acl, acl_type) < 0) {
        warn("failed to set acl entries for %s", to.p_path);
        acl_free(acl);
        return (1);
    }
    acl_free(acl);
#endif
    return (0);
}

int preserve_dir_acls(const struct stat *fs, char *source_dir, char *dest_dir)
{
#if 0
    acl_t (*aclgetf)(const char *, acl_type_t);
    int (*aclsetf)(const char *, acl_type_t, acl_t);
    struct acl *aclp;
    acl_t acl;
    acl_type_t acl_type;
    int acl_supported = 0, ret, trivial;

    ret = pathconf(source_dir, _PC_ACL_NFS4);
    if (ret > 0) {
        acl_supported = 1;
        acl_type = ACL_TYPE_NFS4;
    } else if (ret < 0 && errno != EINVAL) {
        warn("fpathconf(..., _PC_ACL_NFS4) failed for %s", source_dir);
        return (1);
    }
    if (acl_supported == 0) {
        ret = pathconf(source_dir, _PC_ACL_EXTENDED);
        if (ret > 0) {
            acl_supported = 1;
            acl_type = ACL_TYPE_ACCESS;
        } else if (ret < 0 && errno != EINVAL) {
            warn("fpathconf(..., _PC_ACL_EXTENDED) failed for %s",
                 source_dir);
            return (1);
        }
    }
    if (acl_supported == 0)
        return (0);

    /*
     * If the file is a link we will not follow it
     */
    if (S_ISLNK(fs->st_mode)) {
        aclgetf = acl_get_link_np;
        aclsetf = acl_set_link_np;
    } else {
        aclgetf = acl_get_file;
        aclsetf = acl_set_file;
    }
    if (acl_type == ACL_TYPE_ACCESS) {
        /*
         * Even if there is no ACL_TYPE_DEFAULT entry here, a zero
         * size ACL will be returned. So it is not safe to simply
         * check the pointer to see if the default ACL is present.
         */
        acl = aclgetf(source_dir, ACL_TYPE_DEFAULT);
        if (acl == NULL) {
            warn("failed to get default acl entries on %s",
                 source_dir);
            return (1);
        }
        aclp = &acl->ats_acl;
        if (aclp->acl_cnt != 0 && aclsetf(dest_dir,
                                          ACL_TYPE_DEFAULT, acl) < 0) {
            warn("failed to set default acl entries on %s",
                 dest_dir);
            acl_free(acl);
            return (1);
        }
        acl_free(acl);
    }
    acl = aclgetf(source_dir, acl_type);
    if (acl == NULL) {
        warn("failed to get acl entries on %s", source_dir);
        return (1);
    }
    if (acl_is_trivial_np(acl, &trivial)) {
        warn("acl_is_trivial() failed on %s", source_dir);
        acl_free(acl);
        return (1);
    }
    if (trivial) {
        acl_free(acl);
        return (0);
    }
    if (aclsetf(dest_dir, acl_type, acl) < 0) {
        warn("failed to set acl entries on %s", dest_dir);
        acl_free(acl);
        return (1);
    }
    acl_free(acl);
#endif
    return (0);
}
