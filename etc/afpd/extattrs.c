/*
  $Id: extattrs.c,v 1.4 2009-10-02 09:32:40 franklahm Exp $
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#include <atalk/adouble.h>
#include <atalk/vfs.h>
#include <atalk/afp.h>
#include <atalk/logger.h>
#include <atalk/ea.h>

#include "globals.h"
#include "volume.h"
#include "desktop.h"
#include "directory.h"
#include "fork.h"
#include "extattrs.h"

static char *ea_finderinfo = "com.apple.FinderInfo";
static char *ea_resourcefork = "com.apple.ResourceFork";

/* This should be big enough to consecutively store the names of all attributes */
static char attrnamebuf[ATTRNAMEBUFSIZ];

static void hexdump(void *m, size_t l) {
    char *p = m;
    int count = 0, len;
    char buf[100];
    char *bufp = buf;

    while (l--) {
        len = sprintf(bufp, "%02x ", *p++);
        bufp += len;
        count++;

        if ((count % 16) == 0) {
            LOG(log_debug9, logtype_afpd, "%s", buf);
            bufp = buf;
        }
    }
}

/***************************************
 * AFP funcs
 ****************************************/

/*
  Note: we're being called twice. Firstly the client only want the size of all
  EA names, secondly it wants these names. In order to avoid scanning EAs twice
  we cache them in a static buffer.
*/
int afp_listextattr(AFPObj *obj, char *ibuf, int ibuflen _U_, char *rbuf, int *rbuflen)
{
    int                 count, ret, oflag = 0;
    uint16_t            vid, bitmap;
    uint32_t            did, maxreply, tmpattr;
    struct vol          *vol;
    struct dir          *dir;
    struct path         *s_path;
    struct adouble      ad, *adp = NULL;
    struct ofork        *of;
    char                *uname, *FinderInfo;
    static int          buf_valid = 0, attrbuflen = 0;

    *rbuflen = 0;
    ibuf += 2;

    /* Get MaxReplySize first */
    memcpy( &maxreply, ibuf + 14, 4);
    maxreply = ntohl( maxreply );

    /*
      If its the first request with maxreply=0 or if we didn't mark our buffers valid for
      whatever reason (just a safety check, it should be valid), then scan for attributes
    */
    if ((maxreply == 0) || (buf_valid == 0)) {

        attrbuflen = 0;

        memcpy( &vid, ibuf, 2);
        ibuf += 2;
        if (NULL == ( vol = getvolbyvid( vid )) ) {
            LOG(log_error, logtype_afpd, "afp_listextattr: getvolbyvid error: %s", strerror(errno));
            return AFPERR_ACCESS;
        }

        memcpy( &did, ibuf, 4);
        ibuf += 4;
        if (NULL == ( dir = dirlookup( vol, did )) ) {
            LOG(log_error, logtype_afpd, "afp_listextattr: dirlookup error: %s", strerror(errno));
            return afp_errno;
        }

        memcpy( &bitmap, ibuf, 2);
        bitmap = ntohs( bitmap );
        ibuf += 2;

#ifdef HAVE_SOLARIS_EAS
        if (bitmap & kXAttrNoFollow)
            oflag = O_NOFOLLOW;
#endif
        /* Skip ReqCount, StartIndex and maxreply*/
        ibuf += 10;

        /* get name */
        if (NULL == ( s_path = cname( vol, dir, &ibuf )) ) {
            LOG(log_error, logtype_afpd, "afp_listextattr: cname error: %s", strerror(errno));
            return AFPERR_NOOBJ;
        }
        uname = s_path->u_name;

        /*
          We have to check the FinderInfo for the file, because if they aren't all 0
          we must return the synthetic attribute "com.apple.FinderInfo".
          Note: the client will never (never seen in traces) request that attribute
          via FPGetExtAttr !
        */
        if ((of = of_findname(s_path))) {
            adp = of->of_ad;
        } else {
            ad_init(&ad, vol->v_adouble, vol->v_ad_options);
            adp = &ad;
        }

        if ( ad_metadata( uname, 0, adp) < 0 ) {
            switch (errno) {
            case EACCES:
                LOG(log_error, logtype_afpd, "afp_listextattr(%s): %s: check resource fork permission?",
                    uname, strerror(errno));
                return AFPERR_ACCESS;
            default:
                LOG(log_error, logtype_afpd, "afp_listextattr(%s): error getting metadata: %s", uname, strerror(errno));
                return AFPERR_MISC;
            }
        }

        FinderInfo = ad_entry(adp, ADEID_FINDERI);
#ifdef DEBUG
        LOG(log_debug9, logtype_afpd, "afp_listextattr(%s): FinderInfo:", uname);
        hexdump( FinderInfo, 32);
#endif

        /* Now scan FinderInfo if its all 0 */
        count = 32;
        while (count--) {
            if (*FinderInfo++) {
                /* FinderInfo contains some non 0 bytes -> include "com.apple.FinderInfo" */
                strcpy(attrnamebuf, ea_finderinfo);
                attrbuflen += strlen(ea_finderinfo) + 1;
                LOG(log_debug7, logtype_afpd, "afp_listextattr(%s): sending com.apple.FinderInfo", uname);
                break;
            }
        }

        /* Now check for Ressource fork and add virtual EA "com.apple.ResourceFork" if size > 0 */
        LOG(log_debug7, logtype_afpd, "afp_listextattr(%s): Ressourcefork size: %u", uname, adp->ad_eid[ADEID_RFORK].ade_len);
        if (adp->ad_eid[ADEID_RFORK].ade_len > 0) {
            LOG(log_debug7, logtype_afpd, "afp_listextattr(%s): sending com.apple.RessourceFork.", uname);
            strcpy(attrnamebuf + attrbuflen, ea_resourcefork);
            attrbuflen += strlen(ea_resourcefork) + 1;
        }

        ret = vol->vfs->list_eas(vol, attrnamebuf, &attrbuflen, uname, oflag);

        switch (ret) {
        case AFPERR_BADTYPE:
            /* its a symlink and client requested O_NOFOLLOW */
            LOG(log_debug, logtype_afpd, "afp_listextattr(%s): encountered symlink with kXAttrNoFollow", uname);
            attrbuflen = 0;
            buf_valid = 0;
            ret = AFP_OK;
            goto exit;
        case AFPERR_MISC:
            attrbuflen = 0;
            goto exit;
        default:
            buf_valid = 1;
        }
    }

    /* Start building reply packet */
    bitmap = htons(bitmap);
    memcpy( rbuf, &bitmap, 2);
    rbuf += 2;
    *rbuflen += 2;

    tmpattr = htonl(attrbuflen);
    memcpy( rbuf, &tmpattr, 4);
    rbuf += 4;
    *rbuflen += 4;

    /* Only copy buffer if the client asked for it (2nd request, maxreply>0)
       and we didnt have an error (buf_valid) */
    if (maxreply && buf_valid) {
        memcpy( rbuf, attrnamebuf, attrbuflen);
        *rbuflen += attrbuflen;
        buf_valid = 0;
    }

    ret = AFP_OK;

exit:
    if (ret != AFP_OK)
        buf_valid = 0;
    if (adp)
        ad_close_metadata( adp);

    return ret;
}

int afp_getextattr(AFPObj *obj _U_, char *ibuf, int ibuflen _U_, char *rbuf, int *rbuflen)
{
    int                 ret, oflag = 0;
    uint16_t            vid, bitmap;
    uint32_t            did, maxreply, attrnamelen;
    char                attrmname[256], attruname[256];
    struct vol          *vol;
    struct dir          *dir;
    struct path         *s_path;


    *rbuflen = 0;
    ibuf += 2;

    memcpy( &vid, ibuf, 2);
    ibuf += 2;
    if (NULL == ( vol = getvolbyvid( vid )) ) {
        LOG(log_error, logtype_afpd, "afp_getextattr: getvolbyvid error: %s", strerror(errno));
        return AFPERR_ACCESS;
    }

    memcpy( &did, ibuf, 4);
    ibuf += 4;
    if (NULL == ( dir = dirlookup( vol, did )) ) {
        LOG(log_error, logtype_afpd, "afp_getextattr: dirlookup error: %s", strerror(errno));
        return afp_errno;
    }

    memcpy( &bitmap, ibuf, 2);
    bitmap = ntohs( bitmap );
    ibuf += 2;

#ifdef HAVE_SOLARIS_EAS
    if (bitmap & kXAttrNoFollow)
        oflag = O_NOFOLLOW;
#endif

    /* Skip Offset and ReqCount */
    ibuf += 16;

    /* Get MaxReply */
    memcpy(&maxreply, ibuf, 4);
    maxreply = ntohl(maxreply);
    ibuf += 4;

    /* get name */
    if (NULL == ( s_path = cname( vol, dir, &ibuf )) ) {
        LOG(log_error, logtype_afpd, "afp_getextattr: cname error: %s", strerror(errno));
        return AFPERR_NOOBJ;
    }

    if ((unsigned long)ibuf & 1)
        ibuf++;

    /* get length of EA name */
    memcpy(&attrnamelen, ibuf, 2);
    attrnamelen = ntohs(attrnamelen);
    ibuf += 2;
    if (attrnamelen > 255)
        /* dont fool with us */
        attrnamelen = 255;

    /* we must copy the name as its not 0-terminated and I DONT WANT TO WRITE to ibuf */
    strncpy(attrmname, ibuf, attrnamelen);
    attrmname[attrnamelen] = 0;

    LOG(log_debug, logtype_afpd, "afp_getextattr(%s): EA: %s", s_path->u_name, attrmname);

    /* Convert EA name in utf8 to unix charset */
    if ( 0 >= ( attrnamelen = convert_string(CH_UTF8_MAC, obj->options.unixcharset,attrmname, attrnamelen, attruname, 255)) )
        return AFPERR_MISC;

    if (attrnamelen == 255)
        /* convert_string didn't 0-terminate */
        attruname[255] = 0;

    /* write bitmap now */
    bitmap = htons(bitmap);
    memcpy(rbuf, &bitmap, 2);
    rbuf += 2;
    *rbuflen += 2;

    /*
      Switch on maxreply:
      if its 0 we must return the size of the requested attribute,
      if its non 0 we must return the attribute.
    */
    if (maxreply == 0)
        ret = vol->vfs->get_easize(vol, rbuf, rbuflen, s_path->u_name, oflag, attruname);
    else
        ret = vol->vfs->get_eacontent(vol, rbuf, rbuflen, s_path->u_name, oflag, attruname, maxreply);

    return ret;
}

int afp_setextattr(AFPObj *obj _U_, char *ibuf, int ibuflen _U_, char *rbuf, int *rbuflen)
{
    int                 oflag = O_CREAT | O_WRONLY, ret;
    uint16_t            vid, bitmap;
    uint32_t            did, attrnamelen, attrsize;
    char                attrmname[256], attruname[256];
    struct vol          *vol;
    struct dir          *dir;
    struct path         *s_path;

    *rbuflen = 0;
    ibuf += 2;

    memcpy( &vid, ibuf, 2);
    ibuf += 2;
    if (NULL == ( vol = getvolbyvid( vid )) ) {
        LOG(log_error, logtype_afpd, "afp_setextattr: getvolbyvid error: %s", strerror(errno));
        return AFPERR_ACCESS;
    }

    memcpy( &did, ibuf, 4);
    ibuf += 4;
    if (NULL == ( dir = dirlookup( vol, did )) ) {
        LOG(log_error, logtype_afpd, "afp_setextattr: dirlookup error: %s", strerror(errno));
        return afp_errno;
    }

    memcpy( &bitmap, ibuf, 2);
    bitmap = ntohs( bitmap );
    ibuf += 2;

#ifdef HAVE_SOLARIS_EAS
    if (bitmap & kXAttrNoFollow)
        oflag |= AT_SYMLINK_NOFOLLOW;
#endif

    if (bitmap & kXAttrCreate)
        oflag |= O_EXCL;
    else if (bitmap & kXAttrReplace)
        oflag |= O_TRUNC;

    /* Skip Offset */
    ibuf += 8;

    /* get name */
    if (NULL == ( s_path = cname( vol, dir, &ibuf )) ) {
        LOG(log_error, logtype_afpd, "afp_setextattr: cname error: %s", strerror(errno));
        return AFPERR_NOOBJ;
    }

    if ((unsigned long)ibuf & 1)
        ibuf++;

    /* get length of EA name */
    memcpy(&attrnamelen, ibuf, 2);
    attrnamelen = ntohs(attrnamelen);
    ibuf += 2;
    if (attrnamelen > 255)
        return AFPERR_PARAM;

    /* we must copy the name as its not 0-terminated and we cant write to ibuf */
    strncpy(attrmname, ibuf, attrnamelen);
    attrmname[attrnamelen] = 0;
    ibuf += attrnamelen;

    /* Convert EA name in utf8 to unix charset */
    if ( 0 >= ( attrnamelen = convert_string(CH_UTF8_MAC, obj->options.unixcharset,attrmname, attrnamelen, attruname, 255)) )
        return AFPERR_MISC;

    if (attrnamelen == 255)
        /* convert_string didn't 0-terminate */
        attruname[255] = 0;

    /* get EA size */
    memcpy(&attrsize, ibuf, 4);
    attrsize = ntohl(attrsize);
    ibuf += 4;
    if (attrsize > MAX_EA_SIZE)
        /* we arbitrarily make this fatal */
        return AFPERR_PARAM;

    LOG(log_debug, logtype_afpd, "afp_setextattr(%s): EA: %s, size: %u", s_path->u_name, attrmname, attrsize);

    ret = vol->vfs->set_ea(vol, s_path->u_name, attruname, ibuf, attrsize, oflag);

    return ret;
}

int afp_remextattr(AFPObj *obj _U_, char *ibuf, int ibuflen _U_, char *rbuf, int *rbuflen)
{
    int                 oflag = O_RDONLY, ret;
    uint16_t            vid, bitmap;
    uint32_t            did, attrnamelen;
    char                attrmname[256], attruname[256];
    struct vol          *vol;
    struct dir          *dir;
    struct path         *s_path;

    *rbuflen = 0;
    ibuf += 2;

    memcpy( &vid, ibuf, 2);
    ibuf += 2;
    if (NULL == ( vol = getvolbyvid( vid )) ) {
        LOG(log_error, logtype_afpd, "afp_remextattr: getvolbyvid error: %s", strerror(errno));
        return AFPERR_ACCESS;
    }

    memcpy( &did, ibuf, 4);
    ibuf += 4;
    if (NULL == ( dir = dirlookup( vol, did )) ) {
        LOG(log_error, logtype_afpd, "afp_remextattr: dirlookup error: %s", strerror(errno));
        return afp_errno;
    }

    memcpy( &bitmap, ibuf, 2);
    bitmap = ntohs( bitmap );
    ibuf += 2;

#ifdef HAVE_SOLARIS_EAS
    if (bitmap & kXAttrNoFollow)
        oflag |= AT_SYMLINK_NOFOLLOW;
#endif

    /* get name */
    if (NULL == ( s_path = cname( vol, dir, &ibuf )) ) {
        LOG(log_error, logtype_afpd, "afp_setextattr: cname error: %s", strerror(errno));
        return AFPERR_NOOBJ;
    }

    if ((unsigned long)ibuf & 1)
        ibuf++;

    /* get length of EA name */
    memcpy(&attrnamelen, ibuf, 2);
    attrnamelen = ntohs(attrnamelen);
    ibuf += 2;
    if (attrnamelen > 255)
        return AFPERR_PARAM;

    /* we must copy the name as its not 0-terminated and we cant write to ibuf */
    strncpy(attrmname, ibuf, attrnamelen);
    attrmname[attrnamelen] = 0;
    ibuf += attrnamelen;

    /* Convert EA name in utf8 to unix charset */
    if ( 0 >= ( attrnamelen = convert_string(CH_UTF8_MAC, obj->options.unixcharset,attrmname, attrnamelen, attruname, 255)) )
        return AFPERR_MISC;

    if (attrnamelen == 255)
        /* convert_string didn't 0-terminate */
        attruname[255] = 0;

    LOG(log_debug, logtype_afpd, "afp_remextattr(%s): EA: %s", s_path->u_name, attrmname);

    ret = vol->vfs->remove_ea(vol, s_path->u_name, attruname, oflag);

    return ret;
}

