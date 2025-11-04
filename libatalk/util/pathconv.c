/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/util.h>
#include <atalk/volume.h>

/*!
 * @brief Convert a UTF-8 filename to a mangled Mac filename
 *
 * @param vol volume structure
 * @param upath input path in UTF-8
 *
 * @return pointer to static buffer containing mangled Mac filename
 *
 * @sa utompath() in etc/afpd/desktop.c
 */
char *convert_utf8_to_mac(const struct vol *vol, const char *upath)
{
    /* for convert_charset dest_len parameter +2 */
    static char  mpath[MAXPATHLEN + 2];
    char         *m;
    const char   *u;
    uint16_t     flags = CONV_IGNORE | CONV_UNESCAPEHEX;
    size_t       outlen;

    if (!upath) {
        return NULL;
    }

    m = mpath;
    u = upath;
    outlen = strnlen(upath, MAXPATHLEN);

    if (vol->v_casefold & AFPVOL_UTOMUPPER) {
        flags |= CONV_TOUPPER;
    } else if (vol->v_casefold & AFPVOL_UTOMLOWER) {
        flags |= CONV_TOLOWER;
    }

    if (vol->v_flags & AFPVOL_EILSEQ) {
        flags |= CONV__EILSEQ;
    }

    /* convert charsets */
    if ((size_t) -1 == convert_charset(vol->v_volcharset,
                                       CH_UTF8_MAC,
                                       vol->v_maccharset,
                                       u, outlen, mpath, MAXPATHLEN, &flags)) {
        LOG(log_error, logtype_default, "Conversion from %s to %s for %s failed.",
            vol->v_volcodepage, vol->v_maccodepage, u);
        return NULL;
    }

    return m;
}