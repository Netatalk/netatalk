/*
  $Id: ea_solaris.c,v 1.3 2009-10-29 13:06:19 franklahm Exp $
  Copyright (c) 2009 Frank Lahm <franklahm@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/

/* According to man fsattr.5 we must define _ATFILE_SOURCE */
#define _ATFILE_SOURCE

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#include <atalk/adouble.h>
#include <atalk/ea.h>
#include <atalk/afp.h>
#include <atalk/logger.h>
#include <atalk/volume.h>
#include <atalk/vfs.h>
#include <atalk/util.h>
#include <atalk/unix.h>


/**********************************************************************************
 * Solaris EA VFS funcs
 **********************************************************************************/

/*
 * Function: sol_get_easize
 *
 * Purpose: get size of an EA on Solaris native EA
 *
 * Arguments:
 *
 *    vol          (r) current volume
 *    rbuf         (w) DSI reply buffer
 *    rbuflen      (rw) current length of data in reply buffer
 *    uname        (r) filename
 *    oflag        (r) link and create flag
 *    attruname    (r) name of attribute
 *
 * Returns: AFP code: AFP_OK on success or appropiate AFP error code
 *
 * Effects:
 *
 * Copies EA size into rbuf in network order. Increments *rbuflen +4.
 */
int sol_get_easize(VFS_FUNC_ARGS_EA_GETSIZE)
{
    int                 ret, attrdirfd;
    uint32_t            attrsize;
    struct stat         st;

    LOG(log_debug7, logtype_afpd, "sol_getextattr_size(%s): attribute: \"%s\"", uname, attruname);

    if ( -1 == (attrdirfd = attropen(uname, ".", O_RDONLY | oflag))) {
        if (errno == ELOOP) {
            /* its a symlink and client requested O_NOFOLLOW  */
            LOG(log_debug, logtype_afpd, "sol_getextattr_size(%s): encountered symlink with kXAttrNoFollow", uname);

            memset(rbuf, 0, 4);
            *rbuflen += 4;

            return AFP_OK;
        }
        LOG(log_error, logtype_afpd, "sol_getextattr_size: attropen error: %s", strerror(errno));
        return AFPERR_MISC;
    }

    if ( -1 == (fstatat(attrdirfd, attruname, &st, 0))) {
        LOG(log_error, logtype_afpd, "sol_getextattr_size: fstatat error: %s", strerror(errno));
        ret = AFPERR_MISC;
        goto exit;
    }
    attrsize = (st.st_size > MAX_EA_SIZE) ? MAX_EA_SIZE : st.st_size;

    /* Start building reply packet */

    LOG(log_debug7, logtype_afpd, "sol_getextattr_size(%s): attribute: \"%s\", size: %u", uname, attruname, attrsize);

    /* length of attribute data */
    attrsize = htonl(attrsize);
    memcpy(rbuf, &attrsize, 4);
    *rbuflen += 4;

    ret = AFP_OK;

exit:
    close(attrdirfd);
    return ret;
}

/*
 * Function: sol_get_eacontent
 *
 * Purpose: copy Solaris native EA into rbuf
 *
 * Arguments:
 *
 *    vol          (r) current volume
 *    rbuf         (w) DSI reply buffer
 *    rbuflen      (rw) current length of data in reply buffer
 *    uname        (r) filename
 *    oflag        (r) link and create flag
 *    attruname    (r) name of attribute
 *    maxreply     (r) maximum EA size as of current specs/real-life
 *
 * Returns: AFP code: AFP_OK on success or appropiate AFP error code
 *
 * Effects:
 *
 * Copies EA into rbuf. Increments *rbuflen accordingly.
 */
int sol_get_eacontent(VFS_FUNC_ARGS_EA_GETCONTENT)
{
    int                 ret, attrdirfd;
    size_t              toread, okread = 0, len;
    char                *datalength;
    struct stat         st;

    if ( -1 == (attrdirfd = attropen(uname, attruname, O_RDONLY | oflag))) {
        if (errno == ELOOP) {
            /* its a symlink and client requested O_NOFOLLOW  */
            LOG(log_debug, logtype_afpd, "sol_getextattr_content(%s): encountered symlink with kXAttrNoFollow", uname);

            memset(rbuf, 0, 4);
            *rbuflen += 4;

            return AFP_OK;
        }
        LOG(log_error, logtype_afpd, "sol_getextattr_content(%s): attropen error: %s", attruname, strerror(errno));
        return AFPERR_MISC;
    }

    if ( -1 == (fstat(attrdirfd, &st))) {
        LOG(log_error, logtype_afpd, "sol_getextattr_content(%s): fstatat error: %s", attruname,strerror(errno));
        ret = AFPERR_MISC;
        goto exit;
    }

    /* Start building reply packet */

    maxreply -= MAX_REPLY_EXTRA_BYTES;
    if (maxreply > MAX_EA_SIZE)
        maxreply = MAX_EA_SIZE;

    /* But never send more than the client requested */
    toread = (maxreply < st.st_size) ? maxreply : st.st_size;

    LOG(log_debug7, logtype_afpd, "sol_getextattr_content(%s): attribute: \"%s\", size: %u", uname, attruname, maxreply);

    /* remember where we must store length of attribute data in rbuf */
    datalength = rbuf;
    rbuf += 4;
    *rbuflen += 4;

    while (1) {
        len = read(attrdirfd, rbuf, toread);
        if (len == -1) {
            LOG(log_error, logtype_afpd, "sol_getextattr_content(%s): read error: %s", attruname, strerror(errno));
            ret = AFPERR_MISC;
            goto exit;
        }
        okread += len;
        rbuf += len;
        *rbuflen += len;
        if ((len == 0) || (okread == toread))
            break;
    }

    okread = htonl((uint32_t)okread);
    memcpy(datalength, &okread, 4);

    ret = AFP_OK;

exit:
    close(attrdirfd);
    return ret;
}

/*
 * Function: sol_list_eas
 *
 * Purpose: copy names of Solaris native EA into attrnamebuf
 *
 * Arguments:
 *
 *    vol          (r) current volume
 *    attrnamebuf  (w) store names a consecutive C strings here
 *    buflen       (rw) length of names in attrnamebuf
 *    uname        (r) filename
 *    oflag        (r) link and create flag
 *
 * Returns: AFP code: AFP_OK on success or appropiate AFP error code
 *
 * Effects:
 *
 * Copies names of all EAs of uname as consecutive C strings into rbuf.
 * Increments *rbuflen accordingly.
 */
int sol_list_eas(VFS_FUNC_ARGS_EA_LIST)
{
    int ret, attrbuflen = *buflen, len, attrdirfd = 0;
    struct dirent *dp;
    DIR *dirp = NULL;

    /* Now list file attribute dir */
    if ( -1 == (attrdirfd = attropen( uname, ".", O_RDONLY | oflag))) {
        if (errno == ELOOP) {
            /* its a symlink and client requested O_NOFOLLOW */
            ret = AFPERR_BADTYPE;
            goto exit;
        }
        LOG(log_error, logtype_afpd, "sol_list_extattr(%s): error opening atttribute dir: %s", uname, strerror(errno));
        ret = AFPERR_MISC;
        goto exit;
    }

    if (NULL == (dirp = fdopendir(attrdirfd))) {
        LOG(log_error, logtype_afpd, "sol_list_extattr(%s): error opening atttribute dir: %s", uname, strerror(errno));
        ret = AFPERR_MISC;
        goto exit;
    }

    while ((dp = readdir(dirp)))  {
        /* check if its "." or ".." */
        if ((strcmp(dp->d_name, ".") == 0) || (strcmp(dp->d_name, "..") == 0) ||
            (strcmp(dp->d_name, "SUNWattr_ro") == 0) || (strcmp(dp->d_name, "SUNWattr_rw") == 0))
            continue;

        len = strlen(dp->d_name);

        /* Convert name to CH_UTF8_MAC and directly store in in the reply buffer */
        if ( 0 >= ( len = convert_string(vol->v_volcharset, CH_UTF8_MAC, dp->d_name, len, attrnamebuf + attrbuflen, 255)) ) {
            ret = AFPERR_MISC;
            goto exit;
        }
        if (len == 255)
            /* convert_string didn't 0-terminate */
            attrnamebuf[attrbuflen + 255] = 0;

        LOG(log_debug7, logtype_afpd, "sol_list_extattr(%s): attribute: %s", uname, dp->d_name);

        attrbuflen += len + 1;
        if (attrbuflen > (ATTRNAMEBUFSIZ - 256)) {
            /* Next EA name could overflow, so bail out with error.
               FIXME: evantually malloc/memcpy/realloc whatever.
               Is it worth it ? */
            LOG(log_warning, logtype_afpd, "sol_list_extattr(%s): running out of buffer for EA names", uname);
            ret = AFPERR_MISC;
            goto exit;
        }
    }

    ret = AFP_OK;

exit:
    if (dirp)
        closedir(dirp);

    if (attrdirfd > 0)
        close(attrdirfd);

    *buflen = attrbuflen;
    return ret;
}

/*
 * Function: sol_set_ea
 *
 * Purpose: set a Solaris native EA
 *
 * Arguments:
 *
 *    vol          (r) current volume
 *    uname        (r) filename
 *    attruname    (r) EA name
 *    ibuf         (r) buffer with EA content
 *    attrsize     (r) length EA in ibuf
 *    oflag        (r) link and create flag
 *
 * Returns: AFP code: AFP_OK on success or appropiate AFP error code
 *
 * Effects:
 *
 * Copies names of all EAs of uname as consecutive C strings into rbuf.
 * Increments *rbuflen accordingly.
 */
int sol_set_ea(VFS_FUNC_ARGS_EA_SET)
{
    int attrdirfd;

    if ( -1 == (attrdirfd = attropen(uname, attruname, oflag, 0666))) {
        if (errno == ELOOP) {
            /* its a symlink and client requested O_NOFOLLOW  */
            LOG(log_debug, logtype_afpd, "afp_setextattr(%s): encountered symlink with kXAttrNoFollow", uname);
            return AFP_OK;
        }
        LOG(log_error, logtype_afpd, "afp_setextattr(%s): attropen error: %s", uname, strerror(errno));
        return AFPERR_MISC;
    }

    if ((write(attrdirfd, ibuf, attrsize)) != attrsize) {
        LOG(log_error, logtype_afpd, "afp_setextattr(%s): read error: %s", attruname, strerror(errno));
        return AFPERR_MISC;
    }

    return AFP_OK;
}

/*
 * Function: sol_remove_ea
 *
 * Purpose: remove a Solaris native EA
 *
 * Arguments:
 *
 *    vol          (r) current volume
 *    uname        (r) filename
 *    attruname    (r) EA name
 *    oflag        (r) link and create flag
 *
 * Returns: AFP code: AFP_OK on success or appropiate AFP error code
 *
 * Effects:
 *
 * Removes EA attruname from file uname.
 */
int sol_remove_ea(VFS_FUNC_ARGS_EA_REMOVE)
{
    int attrdirfd;

    if ( -1 == (attrdirfd = attropen(uname, ".", oflag))) {
        switch (errno) {
        case ELOOP:
            /* its a symlink and client requested O_NOFOLLOW  */
            LOG(log_debug, logtype_afpd, "afp_remextattr(%s): encountered symlink with kXAttrNoFollow", uname);
            return AFP_OK;
        case EACCES:
            LOG(log_debug, logtype_afpd, "afp_remextattr(%s): unlinkat error: %s", uname, strerror(errno));
            return AFPERR_ACCESS;
        default:
            LOG(log_error, logtype_afpd, "afp_remextattr(%s): attropen error: %s", uname, strerror(errno));
            return AFPERR_MISC;
        }
    }

    if ( -1 == (unlinkat(attrdirfd, attruname, 0)) ) {
        if (errno == EACCES) {
            LOG(log_debug, logtype_afpd, "afp_remextattr(%s): unlinkat error: %s", uname, strerror(errno));
            return AFPERR_ACCESS;
        }
        LOG(log_error, logtype_afpd, "afp_remextattr(%s): unlinkat error: %s", uname, strerror(errno));
        return AFPERR_MISC;
    }

    return AFP_OK;
}
