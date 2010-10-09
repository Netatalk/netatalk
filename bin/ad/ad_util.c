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
#include <atalk/bstrlib.h>
#include <atalk/bstradd.h>
#include <atalk/util.h>
#include <atalk/logger.h>
#include <atalk/errchk.h>
#include "ad.h"

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

/*!
 * Load volinfo and initialize struct vol
 *
 * @param path   (r)  path to evaluate
 * @param vol    (rw) structure to initialize 
*
 * @returns 0 on success, exits on error
 */
int openvol(const char *path, afpvol_t *vol)
{
    int flags = 0;

    memset(vol, 0, sizeof(afpvol_t));

    /* try to find a .AppleDesktop/.volinfo */
    if (loadvolinfo((char *)path, &vol->volinfo) == 0) {

        if (vol_load_charsets(&vol->volinfo) == -1)
            ERROR("Error loading charsets!");

        /* Sanity checks to ensure we can touch this volume */
        if (vol->volinfo.v_adouble != AD_VERSION2)
            ERROR("Unsupported adouble versions: %u", vol->volinfo.v_adouble);

        if (vol->volinfo.v_vfs_ea != AFPVOL_EA_SYS)
            ERROR("Unsupported Extended Attributes option: %u", vol->volinfo.v_vfs_ea);

        /* initialize sufficient struct vol for VFS initialisation */
        vol->volume.v_adouble = AD_VERSION2;
        vol->volume.v_vfs_ea = AFPVOL_EA_SYS;
        initvol_vfs(&vol->volume);

        if ((vol->volinfo.v_flags & AFPVOL_NODEV))
            flags |= CNID_FLAG_NODEV;

        if ((vol->volume.v_cdb = cnid_open(vol->volinfo.v_dbpath,
                                           0000,
                                           "dbd",
                                           flags,
                                           vol->volinfo.v_dbd_host,
                                           vol->volinfo.v_dbd_port)) == NULL)
            ERROR("Cant initialize CNID database connection for %s", vol->volinfo.v_path);

        cnid_getstamp(vol->volume.v_cdb,
                      vol->db_stamp,
                      sizeof(vol->db_stamp));
    }

    return 0;
}

void closevol(afpvol_t *vol)
{
    if (vol->volume.v_cdb)
        cnid_close(vol->volume.v_cdb);

    memset(vol, 0, sizeof(afpvol_t));
}

/*
  Taken form afpd/desktop.c
*/
char *utompath(const struct volinfo *volinfo, const char *upath)
{
    static char  mpath[ MAXPATHLEN + 2]; /* for convert_charset dest_len parameter +2 */
    char         *m;
    const char   *u;
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
 * Build path relativ to volume root
 *
 * path might be:
 * (a) relative:
 *     "dir/subdir" with cwd: "/afp_volume/topdir"
 * (b) absolute:
 *     "/afp_volume/dir/subdir"
 *
 * @param path     (r) path relative to cwd() or absolute
 * @param volpath  (r) volume path that path is a subdir of (has been computed in volinfo funcs) 
 *
 * @returns relative path in new bstring, caller must bdestroy it
 */
static bstring rel_path_in_vol(const char *path, const char *volpath)
{
    EC_INIT;

    if (path == NULL || volpath == NULL)
        return NULL;

    bstring fpath = NULL;

    /* Make path absolute by concetanating for case (a) */
    if (path[0] != '/') {
        EC_NULL(fpath = bfromcstr(getcwdpath()));
        if (bchar(fpath, blength(fpath) - 1) != '/')
            EC_ZERO(bcatcstr(fpath, "/"));
        EC_ZERO(bcatcstr(fpath, path));
        BSTRING_STRIP_SLASH(fpath);
        SLOG("Built path: %s", cfrombstr(fpath));
    } else {
        EC_NULL(fpath = bfromcstr(path));
        BSTRING_STRIP_SLASH(fpath);
        SLOG("Built path: %s", cfrombstr(fpath));
    }

    /*
     * Now we have eg:
     *   fpath:   /Volume/netatalk/dir/bla
     *   volpath: /Volume/netatalk/
     * we want: "dir/bla"
     */
    EC_ZERO(bdelete(fpath, 0, strlen(volpath)));
    SLOG("rel path: %s", cfrombstr(fpath));    
    return fpath;

EC_CLEANUP:
    bdestroy(fpath);
    return NULL;
}

/*!
 * ResolvesCNID of a given paths
 *
 * path might be:
 * (a) relative:
 *     "dir/subdir" with cwd: "/afp_volume/topdir"
 * (b) absolute:
 *     "/afp_volume/dir/subdir"
 *
 * 1) start recursive CNID search with
 *    a) DID:2 / "topdir"
 *    b) DID:2 / "dir"
 * 2) ...until we have the CNID for
 *    a) "/afp_volume/topdir/dir"
 *    b) "/afp_volume/dir" (no recursion required)
 */
cnid_t cnid_for_path(const struct volinfo *vi,
                     const struct vol *vol,
                     const char *path)
{
    EC_INIT;

    cnid_t did;
    cnid_t cnid;
    bstring rpath = NULL;
    bstring statpath = NULL;
    struct bstrList *l = NULL;
    struct stat st;

    EC_NULL(rpath = rel_path_in_vol(path, vi->v_path));
    SLOG("vol:%s, path: %s, rpath: %s", vi->v_path, path, bdata(rpath));

    cnid = htonl(2);

    EC_NULL(statpath = bfromcstr(vi->v_path));

    l = bsplit(rpath, '/');
    SLOG("elem: %s, qty: %u", cfrombstr(l->entry[0]), l->qty);
    for(int i = 0; i < l->qty ; i++) {
        did = cnid;
        EC_ZERO(bconcat(statpath, l->entry[i]));
        SLOG("statpath: %s", cfrombstr(statpath));
        EC_ZERO_LOG(stat(cfrombstr(statpath), &st));
        SLOG("db query: did: %u, name: %s, dev: %08x, ino: %08x",
             ntohl(did), cfrombstr(l->entry[i]), st.st_dev, st.st_ino);
        cnid = cnid_add(vol->v_cdb,
                        &st,
                        did,
                        cfrombstr(l->entry[i]),
                        blength(l->entry[i]),
                        0);

        EC_ZERO(bcatcstr(statpath, "/"));
    }

EC_CLEANUP:
    bdestroy(rpath);
    bstrListDestroy(l);
    bdestroy(statpath);
    if (ret != 0)
        return CNID_INVALID;

    return cnid;
}


