/*
  $Id: extattrs.c,v 1.2 2009-03-03 13:51:25 franklahm Exp $
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

#ifdef HAVE_EXT_ATTRS

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/logger.h>

#include "globals.h"
#include "volume.h"
#include "desktop.h"
#include "directory.h"
#include "fork.h"
#include "extattrs.h"

static char *ea_finderinfo = "com.apple.FinderInfo";
static char *ea_resourcefork = "com.apple.ResourceFork";

/* This should be big enough to consecutively store the names of all attributes */
#define ATTRNAMEBUFSIZ 4096
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

static int getextattr_size(char *rbuf, int *rbuflen, char *uname, int oflag, char *attruname)
{
    int                 ret, attrdirfd;
    uint32_t            attrsize;
    struct stat         st;

    LOG(log_debug7, logtype_afpd, "getextattr_size(%s): attribute: \"%s\"", uname, attruname);

    if ( -1 == (attrdirfd = attropen(uname, ".", O_RDONLY | oflag))) {
        if (errno == ELOOP) {
            /* its a symlink and client requested O_NOFOLLOW  */
            LOG(log_debug, logtype_afpd, "getextattr_size(%s): encountered symlink with kXAttrNoFollow", uname);

	    memset(rbuf, 0, 4);
            *rbuflen += 4;

            return AFP_OK;
        }
        LOG(log_error, logtype_afpd, "getextattr_size: attropen error: %s", strerror(errno));
        return AFPERR_MISC;
    }

    if ( -1 == (fstatat(attrdirfd, attruname, &st, 0))) {
        LOG(log_error, logtype_afpd, "getextattr_size: fstatat error: %s", strerror(errno));
        ret = AFPERR_MISC;
        goto exit;
    }
    attrsize = (st.st_size > MAX_EA_SIZE) ? MAX_EA_SIZE : st.st_size;
    
    /* Start building reply packet */

    LOG(log_debug7, logtype_afpd, "getextattr_size(%s): attribute: \"%s\", size: %u", uname, attruname, attrsize);

    /* length of attribute data */
    attrsize = htonl(attrsize);
    memcpy(rbuf, &attrsize, 4);
    *rbuflen += 4;

    ret = AFP_OK;

exit:
    close(attrdirfd);
    return ret;
}

static int getextattr_content(char *rbuf, int *rbuflen, char *uname, int oflag, char *attruname, int maxreply)
{
    int                 ret, attrdirfd;
    size_t              toread, okread = 0, len;
    char                *datalength;
    struct stat         st;

    if ( -1 == (attrdirfd = attropen(uname, attruname, O_RDONLY | oflag))) {
        if (errno == ELOOP) {
            /* its a symlink and client requested O_NOFOLLOW  */
            LOG(log_debug, logtype_afpd, "getextattr_content(%s): encountered symlink with kXAttrNoFollow", uname);

	    memset(rbuf, 0, 4);
            *rbuflen += 4;

            return AFP_OK;
        }
        LOG(log_error, logtype_afpd, "getextattr_content(%s): attropen error: %s", attruname, strerror(errno));
        return AFPERR_MISC;
    }

    if ( -1 == (fstat(attrdirfd, &st))) {
        LOG(log_error, logtype_afpd, "getextattr_content(%s): fstatat error: %s", attruname,strerror(errno));
        ret = AFPERR_MISC;
        goto exit;
    }

    /* Start building reply packet */

    maxreply -= MAX_REPLY_EXTRA_BYTES;
    if (maxreply > MAX_EA_SIZE)
        maxreply = MAX_EA_SIZE;

    /* But never send more than the client requested */
    toread = (maxreply < st.st_size) ? maxreply : st.st_size;

    LOG(log_debug7, logtype_afpd, "getextattr_content(%s): attribute: \"%s\", size: %u", uname, attruname, maxreply);

    /* remember where we must store length of attribute data in rbuf */
    datalength = rbuf;
    rbuf += 4;
    *rbuflen += 4;

    while (1) {
        len = read(attrdirfd, rbuf, toread);
        if (len == -1) {
            LOG(log_error, logtype_afpd, "getextattr_content(%s): read error: %s", attruname, strerror(errno));
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

int list_extattr(AFPObj *obj, char *attrnamebuf, int *buflen, char *uname, int oflag)
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
	LOG(log_error, logtype_afpd, "list_extattr(%s): error opening atttribute dir: %s", uname, strerror(errno));
	ret = AFPERR_MISC;
	goto exit;
    }

    if (NULL == (dirp = fdopendir(attrdirfd))) {
	LOG(log_error, logtype_afpd, "list_extattr(%s): error opening atttribute dir: %s", uname, strerror(errno));
	ret = AFPERR_MISC;
	goto exit;
    }
    
    while ((dp = readdir(dirp)))  {
	/* check if its "." or ".." */
	if ((strcmp(dp->d_name, ".") == 0) || (strcmp(dp->d_name, "..") == 0) ||
	    (strcmp(dp->d_name, "SUNWattr_ro") == 0) || (strcmp(dp->d_name, "SUNWattr_rw") == 0))
	    continue;
	
	len = strlen(dp->d_name);
	
	if ( 0 >= ( len = convert_string(obj->options.unixcharset, CH_UTF8_MAC, dp->d_name, len, attrnamebuf + attrbuflen, 255)) ) {
	    ret = AFPERR_MISC;
	    goto exit;
	}
	if (len == 255)
	    /* convert_string didn't 0-terminate */
	    attrnamebuf[attrbuflen + 255] = 0;
	
	LOG(log_debug7, logtype_afpd, "list_extattr(%s): attribute: %s", uname, dp->d_name);
	
	attrbuflen += len + 1;
	if (attrbuflen > (ATTRNAMEBUFSIZ - 256)) {
	    /* Next EA name could overflow, so bail out with error.
	       FIXME: evantually malloc/memcpy/realloc whatever.
	       Is it worth it ? */
	    LOG(log_warning, logtype_afpd, "list_extattr(%s): running out of buffer for EA names", uname);
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

/***************************************
 * Interface
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
    uint32_t            did, maxreply;
    struct vol          *vol;
    struct dir          *dir;
    struct path         *s_path;
    struct adouble      ad, *adp = NULL;
    struct ofork        *of;
    char                *uname, *FinderInfo;
    static int          buf_valid = 0, attrbuflen = 0;

#ifdef DEBUG
    LOG(log_debug9, logtype_afpd, "afp_listextattr: BEGIN");
#endif

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

	if (bitmap & kXAttrNoFollow)
	    oflag = O_NOFOLLOW;

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

	ret = list_extattr(obj, attrnamebuf, &attrbuflen, uname, oflag);
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

    attrbuflen = htonl(attrbuflen);
    memcpy( rbuf, &attrbuflen, 4);
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

    LOG(log_debug9, logtype_afpd, "afp_listextattr: END");
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


#ifdef DEBUG
    LOG(log_debug9, logtype_afpd, "afp_getextattr: BEGIN");
#endif

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
    if (bitmap & kXAttrNoFollow)
        oflag = AT_SYMLINK_NOFOLLOW;

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
        ret = getextattr_size(rbuf, rbuflen, s_path->u_name, oflag, attruname);
    else
        ret = getextattr_content(rbuf, rbuflen, s_path->u_name, oflag, attruname, maxreply);

#ifdef DEBUG
    LOG(log_debug9, logtype_afpd, "afp_getextattr: END");
#endif

    return ret;
}

int afp_setextattr(AFPObj *obj _U_, char *ibuf, int ibuflen _U_, char *rbuf, int *rbuflen)
{
    int                 len, oflag = O_CREAT | O_WRONLY, attrdirfd;
    uint16_t            vid, bitmap;
    uint32_t            did, attrnamelen, attrsize;
    char                attrmname[256], attruname[256];
    struct vol          *vol;
    struct dir          *dir;
    struct path         *s_path;

#ifdef DEBUG
    LOG(log_debug9, logtype_afpd, "afp_setextattr: BEGIN");
#endif

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
    if (bitmap & kXAttrNoFollow)
        oflag |= AT_SYMLINK_NOFOLLOW;
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

    if ( -1 == (attrdirfd = attropen(s_path->u_name, attruname, oflag, 0666))) {
        if (errno == ELOOP) {
            /* its a symlink and client requested O_NOFOLLOW  */
            LOG(log_debug, logtype_afpd, "afp_setextattr(%s): encountered symlink with kXAttrNoFollow", s_path->u_name);
            return AFP_OK;
        }
        LOG(log_error, logtype_afpd, "afp_setextattr(%s): attropen error: %s", s_path->u_name, strerror(errno));
        return AFPERR_MISC;
    }
    
    while (1) {
        len = write(attrdirfd, ibuf, attrsize);
        if (len == -1) {
            LOG(log_error, logtype_afpd, "afp_setextattr(%s): read error: %s", attruname, strerror(errno));
            return AFPERR_MISC;
        }
        attrsize -= len;
        ibuf += len;
        if (attrsize == 0)
            break;
    }

#ifdef DEBUG
    LOG(log_debug9, logtype_afpd, "afp_setextattr: END");
#endif

    return AFP_OK;
}

int afp_remextattr(AFPObj *obj _U_, char *ibuf, int ibuflen _U_, char *rbuf, int *rbuflen)
{
    int                 oflag = O_RDONLY, attrdirfd;
    uint16_t            vid, bitmap;
    uint32_t            did, attrnamelen;
    char                attrmname[256], attruname[256];
    struct vol          *vol;
    struct dir          *dir;
    struct path         *s_path;

#ifdef DEBUG
    LOG(log_debug9, logtype_afpd, "afp_remextattr: BEGIN");
#endif

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
    if (bitmap & kXAttrNoFollow)
        oflag |= AT_SYMLINK_NOFOLLOW;

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

    if ( -1 == (attrdirfd = attropen(s_path->u_name, ".", oflag))) {
	switch (errno) {
	case ELOOP:
            /* its a symlink and client requested O_NOFOLLOW  */
            LOG(log_debug, logtype_afpd, "afp_remextattr(%s): encountered symlink with kXAttrNoFollow", s_path->u_name);
            return AFP_OK;
	case EACCES:
	    LOG(log_debug, logtype_afpd, "afp_remextattr(%s): unlinkat error: %s", s_path->u_name, strerror(errno));
	    return AFPERR_ACCESS;
	default:
	    LOG(log_error, logtype_afpd, "afp_remextattr(%s): attropen error: %s", s_path->u_name, strerror(errno));
	    return AFPERR_MISC;
	}
    }
    
    if ( -1 == (unlinkat(attrdirfd, attruname, 0)) ) {
	if (errno == EACCES) {
	    LOG(log_debug, logtype_afpd, "afp_remextattr(%s): unlinkat error: %s", s_path->u_name, strerror(errno));
	    return AFPERR_ACCESS;
	}
        LOG(log_error, logtype_afpd, "afp_remextattr(%s): unlinkat error: %s", s_path->u_name, strerror(errno));
	return AFPERR_MISC;
    }

#ifdef DEBUG
    LOG(log_debug9, logtype_afpd, "afp_remextattr: END");
#endif

    return AFP_OK;
}



#endif /* HAVE_EXT_ATTRS */

