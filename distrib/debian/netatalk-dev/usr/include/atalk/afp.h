/*
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 * All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation, and that the name of The University
 * of Michigan not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. This software is supplied as is without expressed or
 * implied warranties of any kind.
 *
 *	Research Systems Unix Group
 *	The University of Michigan
 *	c/o Mike Clark
 *	535 W. William Street
 *	Ann Arbor, Michigan
 *	+1-313-763-0525
 *	netatalk@itd.umich.edu
 */

#ifndef _ATALK_AFP_H
#define _ATALK_AFP_H 1

#include <sys/types.h>
#include <netatalk/endian.h>

typedef u_int16_t AFPUserBytes;

/* protocols */
#define AFPPROTO_ASP           1
#define AFPPROTO_DSI           2

/* actual transports. the DSI ones (tcp right now) need to be
 * kept in sync w/ <atalk/dsi.h>. 
 * convention: AFPTRANS_* = (1 << DSI_*) 
 */
#define AFPTRANS_NONE          0
#define AFPTRANS_DDP          (1 << 0)
#define AFPTRANS_TCP          (1 << 1)
#define AFPTRANS_ALL          (AFPTRANS_DDP | AFPTRANS_TCP)

/* server flags */
#define AFPSRVRINFO_COPY	 (1<<0)  /* supports copyfile */
#define AFPSRVRINFO_PASSWD	 (1<<1)	 /* supports change password */
#define AFPSRVRINFO_NOSAVEPASSWD (1<<2)  /* don't allow save password */
#define AFPSRVRINFO_SRVMSGS      (1<<3)  /* supports server messages */
#define AFPSRVRINFO_SRVSIGNATURE (1<<4)  /* supports server signature */
#define AFPSRVRINFO_TCPIP        (1<<5)  /* supports tcpip */
#define AFPSRVRINFO_SRVNOTIFY    (1<<6)  /* supports server notifications */ 
#define AFPSRVRINFO_FASTBOZO	 (1<<15) /* fast copying */

#define AFP_OK		0
#define AFPERR_ACCESS	-5000   /* permission denied */
#define AFPERR_AUTHCONT	-5001   /* logincont */
#define AFPERR_BADUAM	-5002   /* uam doesn't exist */
#define AFPERR_BADVERS	-5003   /* bad afp version number */
#define AFPERR_BITMAP	-5004   /* invalid bitmap */
#define AFPERR_CANTMOVE -5005   /* can't move file */
#define AFPERR_DENYCONF	-5006   /* file synchronization locks conflict */
#define AFPERR_DIRNEMPT	-5007   /* directory not empty */
#define AFPERR_DFULL	-5008   /* disk full */
#define AFPERR_EOF	-5009   /* end of file -- catsearch and afp_read */
#define AFPERR_BUSY	-5010   /* FileBusy */
#define AFPERR_FLATVOL  -5011   /* volume doesn't support directories */
#define AFPERR_NOITEM	-5012   /* ItemNotFound */
#define AFPERR_LOCK     -5013   /* LockErr */
#define AFPERR_MISC     -5014   /* misc. err */
#define AFPERR_NLOCK    -5015   /* no more locks */
#define AFPERR_NOSRVR	-5016   /* no response by server at that address */
#define AFPERR_EXIST	-5017   /* object already exists */
#define AFPERR_NOOBJ	-5018   /* object not found */
#define AFPERR_PARAM	-5019   /* parameter error */
#define AFPERR_NORANGE  -5020   /* no range lock */
#define AFPERR_RANGEOVR -5021   /* range overlap */
#define AFPERR_SESSCLOS -5022   /* session closed */
#define AFPERR_NOTAUTH	-5023   /* user not authenticated */
#define AFPERR_NOOP	-5024   /* command not supported */
#define AFPERR_BADTYPE	-5025   /* object is the wrong type */
#define AFPERR_NFILE	-5026   /* too many files open */
#define AFPERR_SHUTDOWN	-5027   /* server is going down */
#define AFPERR_NORENAME -5028   /* can't rename */
#define AFPERR_NODIR	-5029   /* couldn't find directory */
#define AFPERR_ITYPE	-5030   /* wrong icon type */
#define AFPERR_VLOCK	-5031   /* volume locked */
#define AFPERR_OLOCK    -5032   /* object locked */
#define AFPERR_CTNSHRD  -5033   /* share point contains a share point */
#define AFPERR_NOID     -5034   /* file thread not found */
#define AFPERR_EXISTID  -5035   /* file already has an id */
#define AFPERR_DIFFVOL  -5036   /* different volume */
#define AFPERR_CATCHNG  -5037   /* catalog has changed */
#define AFPERR_SAMEOBJ  -5038   /* source file == destination file */
#define AFPERR_BADID    -5039   /* non-existent file id */
#define AFPERR_PWDSAME  -5040   /* same password/can't change password */
#define AFPERR_PWDSHORT -5041   /* password too short */
#define AFPERR_PWDEXPR  -5042   /* password expired */
#define AFPERR_INSHRD   -5043   /* folder being shared is inside a
				   shared folder. may be returned by
				   afpMoveAndRename. */
#define AFPERR_INTRASH  -5044   /* shared folder in trash. */
#define AFPERR_PWDCHNG  -5045   /* password needs to be changed */
#define AFPERR_PWDPOLCY -5046   /* password fails policy check */
#define AFPERR_USRLOGIN -5047   /* user already logged on */

/* AFP Attention Codes -- 4 bits */
#define AFPATTN_SHUTDOWN     (1 << 15)            /* shutdown/disconnect */
#define AFPATTN_CRASH        (1 << 14)            /* server crashed */
#define AFPATTN_MESG         (1 << 13)            /* server has message */
#define AFPATTN_NORECONNECT  (1 << 12)            /* don't reconnect */
/* server notification */
#define AFPATTN_NOTIFY       (AFPATTN_MESG | AFPATTN_NORECONNECT) 

/* extended bitmap -- 12 bits. volchanged is only useful w/ a server
 * notification, and time is only useful for shutdown. */
#define AFPATTN_VOLCHANGED   (1 << 0)             /* volume has changed */
#define AFPATTN_TIME(x)      ((x) & 0xfff)        /* time in minutes */

typedef enum {
  AFPMESG_LOGIN = 0,
  AFPMESG_SERVER = 1
} afpmessage_t;

#endif
