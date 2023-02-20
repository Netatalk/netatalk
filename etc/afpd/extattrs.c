/*
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

#include <atalk/adouble.h>
#include <atalk/util.h>
#include <atalk/vfs.h>
#include <atalk/afp.h>
#include <atalk/logger.h>
#include <atalk/ea.h>
#include <atalk/globals.h>
#include <atalk/netatalk_conf.h>

#include "volume.h"
#include "desktop.h"
#include "directory.h"
#include "fork.h"
#include "extattrs.h"

static const char *ea_finderinfo = "com.apple.FinderInfo";
static const char *ea_resourcefork = "com.apple.ResourceFork";


#ifdef DEBUG
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
#endif

/***************************************
 * AFP funcs
 ****************************************/

/*
  Note: we're being called twice. Firstly the client only want the size of all
  EA names, secondly it wants these names. In order to avoid scanning EAs twice
  we cache them in a static buffer.
*/
int afp_listextattr(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_, char *rbuf, size_t *rbuflen)
{
    int                 ret, oflag = 0, adflags = 0, fd = -1;
    uint16_t            vid, bitmap, uint16;
    uint32_t            did, maxreply, tmpattr;
    struct vol          *vol;
    struct dir          *dir;
    struct path         *s_path;
    struct stat         *st;
    struct adouble      ad, *adp = NULL;
    struct ofork	*opened = NULL;
    char                *uname, *FinderInfo;
    char                emptyFinderInfo[32] = { 0 };
    size_t              attrbuflen = 0;
    bool                close_ad = false;
    char                attrnamebuf[ATTRNAMEBUFSIZ];

    *rbuflen = 0;
    ibuf += 2;

    /* Get Bitmap and MaxReplySize first */
    memcpy(&bitmap, ibuf +6, sizeof(bitmap));
    bitmap = ntohs(bitmap);

    memcpy(&maxreply, ibuf + 14, sizeof (maxreply));
    maxreply = ntohl(maxreply);

    memcpy(&vid, ibuf, sizeof(vid));
    ibuf += sizeof(vid);
    vol = getvolbyvid(vid);
    if (vol == NULL) {
        LOG(log_debug, logtype_afpd, "afp_listextattr: getvolbyvid error: %s", strerror(errno));
        return AFPERR_ACCESS;
    }

    memcpy(&did, ibuf, sizeof(did));
    ibuf += sizeof(did);
    dir = dirlookup(vol, did);
    if (dir == NULL) {
        LOG(log_debug, logtype_afpd, "afp_listextattr: dirlookup error: %s", strerror(errno));
        return afp_errno;
    }

    if (bitmap & kXAttrNoFollow) {
        oflag = O_NOFOLLOW;
    }

    /* Skip Bitmap, ReqCount, StartIndex and maxreply*/
    ibuf += 12;

    /* get name */
    s_path = cname(vol, dir, &ibuf);
    if (s_path == NULL) {
        LOG(log_debug, logtype_afpd, "afp_listextattr: cname error: %s", strerror(errno));
        return AFPERR_NOOBJ;
    }

    st = &s_path->st;
    if (!s_path->st_valid) {
        /* it's a dir in our cache, we didn't stat it, do it now */
        of_statdir(vol, s_path);
    }
    if (s_path->st_errno != 0) {
        return(AFPERR_NOOBJ);
    }

    uname = s_path->u_name;

    /*
     * We have to check the FinderInfo for the file, because if they
     * aren't all 0 we must return the synthetic attribute
     * "com.apple.FinderInfo".  Note: the client will never (never
     * seen in traces) request that attribute via FPGetExtAttr !
    */

    adp = &ad;
    ad_init(adp, vol);

    if (path_isadir(s_path)) {
	    LOG(log_debug, logtype_afpd, "afp_listextattr(%s): is a dir", uname);
        adflags = ADFLAGS_DIR;
	} else {
	    LOG(log_debug, logtype_afpd, "afp_listextattr(%s): is a file", uname);
	    opened = of_findname(vol, s_path);
	    if (opened) {
            adp = opened->of_ad;
            fd = ad_meta_fileno(adp);
	    }
	}

    if (ad_metadata(uname, adflags, adp) != 0 ) {
        switch (errno) {
        case ENOENT:
            break;
        case EACCES:
            LOG(log_error, logtype_afpd, "afp_listextattr(%s): %s: check resource fork permission?",
                uname, strerror(errno));
            return AFPERR_ACCESS;
        default:
            LOG(log_error, logtype_afpd, "afp_listextattr(%s): error getting metadata: %s", uname, strerror(errno));
            return AFPERR_MISC;
        }
    } else {
        close_ad = true;
        FinderInfo = ad_entry(adp, ADEID_FINDERI);
        /* Check if FinderInfo equals default and empty FinderInfo*/
        if (memcmp(FinderInfo, emptyFinderInfo, 32) != 0) {
            /* FinderInfo contains some non 0 bytes -> include "com.apple.FinderInfo" */
            strcpy(attrnamebuf, ea_finderinfo);
            attrbuflen += strlen(ea_finderinfo) + 1;
            LOG(log_debug7, logtype_afpd, "afp_listextattr(%s): sending com.apple.FinderInfo", uname);
        }

        /* Now check for Resource fork and add virtual EA "com.apple.ResourceFork" if size > 0 */
        LOG(log_debug7, logtype_afpd, "afp_listextattr(%s): Resource fork size: %llu", uname, adp->ad_rlen);

        if (adp->ad_rlen > 0) {
            LOG(log_debug7, logtype_afpd, "afp_listextattr(%s): sending com.apple.ResourceFork.", uname);
            strcpy(attrnamebuf + attrbuflen, ea_resourcefork);
            attrbuflen += strlen(ea_resourcefork) + 1;
        }
    }

    ret = vol->vfs->vfs_ea_list(vol, attrnamebuf, &attrbuflen, uname, oflag, fd);
    if (ret != AFP_OK) {
        attrbuflen = 0;

        switch (ret) {
        case AFPERR_BADTYPE:
            /* its a symlink and client requested O_NOFOLLOW */
            LOG(log_debug, logtype_afpd, "afp_listextattr(%s): encountered symlink with kXAttrNoFollow", uname);
            ret = AFP_OK;
            break;

        default:
            break;
        }
    }

exit:
    bitmap = htons(bitmap);
    memcpy( rbuf, &bitmap, sizeof(bitmap));
    rbuf += sizeof(bitmap);
    *rbuflen += sizeof(bitmap);

    tmpattr = htonl(attrbuflen);
    memcpy( rbuf, &tmpattr, sizeof(tmpattr));
    rbuf += sizeof(tmpattr);
    *rbuflen += sizeof(tmpattr);

    if (maxreply > 0) {
        memcpy( rbuf, attrnamebuf, attrbuflen);
        *rbuflen += attrbuflen;
    }

    if (close_ad) {
        ad_close(adp, ADFLAGS_HF);
    }

    return ret;
}

static char *to_stringz(char *ibuf, uint16_t len)
{
static char attrmname[256];

    if (len > 255)
        /* don't fool with us */
        len = 255;

    /* we must copy the name as its not 0-terminated and I DONT WANT TO WRITE to ibuf */
    strlcpy(attrmname, ibuf, len + 1);
    return attrmname;
}

int afp_getextattr(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_, char *rbuf, size_t *rbuflen)
{
    int                 ret, oflag = 0, fd = -1;
    uint16_t            vid, bitmap, attrnamelen;
    uint32_t            did, maxreply;
    char                attruname[256];
    struct vol          *vol;
    struct dir          *dir;
    struct path         *s_path;
    struct adouble	ad, *adp = NULL;
    struct ofork	*opened = NULL;


    *rbuflen = 0;
    ibuf += 2;

    memcpy( &vid, ibuf, sizeof(vid));
    ibuf += sizeof(vid);
    if (NULL == ( vol = getvolbyvid( vid )) ) {
        LOG(log_debug, logtype_afpd, "afp_getextattr: getvolbyvid error: %s", strerror(errno));
        return AFPERR_ACCESS;
    }

    memcpy( &did, ibuf, sizeof(did));
    ibuf += sizeof(did);
    if (NULL == ( dir = dirlookup( vol, did )) ) {
        LOG(log_debug, logtype_afpd, "afp_getextattr: dirlookup error: %s", strerror(errno));
        return afp_errno;
    }

    memcpy( &bitmap, ibuf, sizeof(bitmap));
    bitmap = ntohs( bitmap );
    ibuf += sizeof(bitmap);

    if (bitmap & kXAttrNoFollow)
        oflag = O_NOFOLLOW;

    /* Skip Offset and ReqCount */
    ibuf += 16;

    /* Get MaxReply */
    memcpy(&maxreply, ibuf, sizeof(maxreply));
    maxreply = ntohl(maxreply);
    ibuf += sizeof(maxreply);

    /* get name */
    if (NULL == ( s_path = cname( vol, dir, &ibuf )) ) {
        LOG(log_debug, logtype_afpd, "afp_getextattr: cname error: %s", strerror(errno));
        return AFPERR_NOOBJ;
    }

    if ((unsigned long)ibuf & 1)
        ibuf++;

    /* get length of EA name */
    memcpy(&attrnamelen, ibuf, sizeof(attrnamelen));
    attrnamelen = ntohs(attrnamelen);
    ibuf += sizeof(attrnamelen);

    LOG(log_debug, logtype_afpd, "afp_getextattr(%s): EA: %s", s_path->u_name, to_stringz(ibuf, attrnamelen));

    /* Convert EA name in utf8 to unix charset */
    if ( 0 >= convert_string(CH_UTF8_MAC, obj->options.unixcharset, ibuf, attrnamelen, attruname, 256) )
        return AFPERR_MISC;

    /* write bitmap now */
    bitmap = htons(bitmap);
    memcpy(rbuf, &bitmap, sizeof(bitmap));
    rbuf += sizeof(bitmap);
    *rbuflen += sizeof(bitmap);

    if (path_isadir(s_path)) {
	LOG(log_debug, logtype_afpd, "afp_getextattr(%s): is a dir", s_path->u_name);
    } else {
	LOG(log_debug, logtype_afpd, "afp_getextattr(%s): is a file", s_path->u_name);
	opened = of_findname(vol, s_path);
	if (opened) {
	    adp = opened->of_ad;
	    fd = ad_meta_fileno(adp);
	}
    }

    /*
      Switch on maxreply:
      if its 0 we must return the size of the requested attribute,
      if its non 0 we must return the attribute.
    */
    if (maxreply == 0)
        ret = vol->vfs->vfs_ea_getsize(vol, rbuf, rbuflen, s_path->u_name, oflag, attruname, fd);
    else
        ret = vol->vfs->vfs_ea_getcontent(vol, rbuf, rbuflen, s_path->u_name, oflag, attruname, maxreply, fd);

    return ret;
}

int afp_setextattr(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_, char *rbuf _U_, size_t *rbuflen)
{
    int                 oflag = 0, ret, fd = -1;
    uint16_t            vid, bitmap, attrnamelen;
    uint32_t            did, attrsize;
    char                attruname[256];
    char		*attrmname;
    struct vol          *vol;
    struct dir          *dir;
    struct path         *s_path;
    struct adouble	ad, *adp = NULL;
    struct ofork	*opened = NULL;

    *rbuflen = 0;
    ibuf += 2;

    memcpy( &vid, ibuf, sizeof(vid));
    ibuf += sizeof(vid);
    if (NULL == ( vol = getvolbyvid( vid )) ) {
        LOG(log_debug, logtype_afpd, "afp_setextattr: getvolbyvid error: %s", strerror(errno));
        return AFPERR_ACCESS;
    }

    memcpy( &did, ibuf, sizeof(did));
    ibuf += sizeof(did);
    if (NULL == ( dir = dirlookup( vol, did )) ) {
        LOG(log_debug, logtype_afpd, "afp_setextattr: dirlookup error: %s", strerror(errno));
        return afp_errno;
    }

    memcpy( &bitmap, ibuf, sizeof(bitmap));
    bitmap = ntohs( bitmap );
    ibuf += sizeof(bitmap);

    if (bitmap & kXAttrNoFollow)
        oflag |= O_NOFOLLOW;

    if (bitmap & kXAttrCreate)
        oflag |= O_CREAT;
    else if (bitmap & kXAttrReplace)
        oflag |= O_TRUNC;

    /* Skip Offset */
    ibuf += 8;

    /* get name */
    if (NULL == ( s_path = cname( vol, dir, &ibuf )) ) {
        LOG(log_debug, logtype_afpd, "afp_setextattr: cname error: %s", strerror(errno));
        return AFPERR_NOOBJ;
    }

    if (path_isadir(s_path)) {
	LOG(log_debug, logtype_afpd, "afp_setextattr(%s): is a dir", s_path->u_name);
    } else {
	LOG(log_debug, logtype_afpd, "afp_setextattr(%s): is a file", s_path->u_name);
	opened = of_findname(vol, s_path);
	if (opened) {
	    adp = opened->of_ad;
	    fd = ad_meta_fileno(adp);
	}
    }


    if ((unsigned long)ibuf & 1)
        ibuf++;

    /* get length of EA name */
    memcpy(&attrnamelen, ibuf, sizeof(attrnamelen));
    attrnamelen = ntohs(attrnamelen);
    ibuf += sizeof(attrnamelen);
    if (attrnamelen > 255)
        return AFPERR_PARAM;

    attrmname = ibuf;
    /* Convert EA name in utf8 to unix charset */
    if ( 0 >= convert_string(CH_UTF8_MAC, obj->options.unixcharset, attrmname, attrnamelen, attruname, 256))
        return AFPERR_MISC;

    ibuf += attrnamelen;
    /* get EA size */
    memcpy(&attrsize, ibuf, sizeof(attrsize));
    attrsize = ntohl(attrsize);
    ibuf += sizeof(attrsize);
    if (attrsize > MAX_EA_SIZE)
        /* we arbitrarily make this fatal */
        return AFPERR_PARAM;

    LOG(log_debug, logtype_afpd, "afp_setextattr(%s): EA: %s, size: %u", s_path->u_name, to_stringz(attrmname, attrnamelen), attrsize);

    ret = vol->vfs->vfs_ea_set(vol, s_path->u_name, attruname, ibuf, attrsize, oflag, fd);

    return ret;
}

int afp_remextattr(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_, char *rbuf _U_, size_t *rbuflen)
{
    int                 oflag = 0, ret, fd = -1;
    uint16_t            vid, bitmap, attrnamelen;
    uint32_t            did;
    char                attruname[256];
    struct vol          *vol;
    struct dir          *dir;
    struct path         *s_path;
    struct adouble	ad, *adp = NULL;
    struct ofork	*opened = NULL;

    *rbuflen = 0;
    ibuf += 2;

    memcpy( &vid, ibuf, sizeof(vid));
    ibuf += sizeof(vid);
    if (NULL == ( vol = getvolbyvid( vid )) ) {
        LOG(log_debug, logtype_afpd, "afp_remextattr: getvolbyvid error: %s", strerror(errno));
        return AFPERR_ACCESS;
    }

    memcpy( &did, ibuf, sizeof(did));
    ibuf += sizeof(did);
    if (NULL == ( dir = dirlookup( vol, did )) ) {
        LOG(log_debug, logtype_afpd, "afp_remextattr: dirlookup error: %s", strerror(errno));
        return afp_errno;
    }

    memcpy( &bitmap, ibuf, sizeof(bitmap));
    bitmap = ntohs( bitmap );
    ibuf += sizeof(bitmap);

    if (bitmap & kXAttrNoFollow)
        oflag |= O_NOFOLLOW;

    /* get name */
    if (NULL == ( s_path = cname( vol, dir, &ibuf )) ) {
        LOG(log_debug, logtype_afpd, "afp_remextattr: cname error: %s", strerror(errno));
        return AFPERR_NOOBJ;
    }

    if (path_isadir(s_path)) {
	LOG(log_debug, logtype_afpd, "afp_remextattr(%s): is a dir", s_path->u_name);
    } else {
	LOG(log_debug, logtype_afpd, "afp_remextattr(%s): is a file", s_path->u_name);
	opened = of_findname(vol, s_path);
	if (opened) {
	    adp = opened->of_ad;
	    fd = ad_meta_fileno(adp);
	}
    }

    if ((unsigned long)ibuf & 1)
        ibuf++;

    /* get length of EA name */
    memcpy(&attrnamelen, ibuf, sizeof(attrnamelen));
    attrnamelen = ntohs(attrnamelen);
    ibuf += sizeof(attrnamelen);
    if (attrnamelen > 255)
        return AFPERR_PARAM;

    /* Convert EA name in utf8 to unix charset */
    if ( 0 >= (convert_string(CH_UTF8_MAC, obj->options.unixcharset,ibuf, attrnamelen, attruname, 256)) )
        return AFPERR_MISC;

    LOG(log_debug, logtype_afpd, "afp_remextattr(%s): EA: %s", s_path->u_name, to_stringz(ibuf, attrnamelen));

    ret = vol->vfs->vfs_ea_remove(vol, s_path->u_name, attruname, oflag, fd);

    return ret;
}

