/*
 * $Id: volume.h,v 1.7 2001-09-04 13:52:45 rufustfirefly Exp $
 *
 * Copyright (c) 1990,1994 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifndef AFPD_VOLUME_H
#define AFPD_VOLUME_H 1

#include <sys/cdefs.h>
#include <sys/types.h>
#include <netatalk/endian.h>
#include "globals.h"

#define AFPVOL_NAMELEN   27

struct codepage_hash {
  unsigned char *from, *to;
  struct codepage_hash *next, *prev;
};

union codepage_val {
  struct codepage_hash hash; /* hash for multibyte values */
  unsigned char value; /* single byte value/rule */
};

struct codepage {
  union codepage_val *map;
  int quantum;
};

#define CP_HASH(a)    (*(a))

struct vol {
    struct vol		*v_next;
    char		*v_name;
    char		*v_path;
    struct dir		*v_dir, *v_root;
    int			v_flags;
#ifdef __svr4__
    int			v_qfd;
#endif /*__svr4__*/
    char		*v_gvs;
    u_int32_t		v_time;
    int			v_lastdid;
    u_int16_t		v_vid;
    void                *v_nfsclient;
    int                 v_nfs, v_casefold;
    struct codepage     *v_mtoupage, *v_utompage, *v_badumap;
    char                *v_password;
    char                *v_veto;
#ifdef CNID_DB
    void                *v_db;
    char                *v_dbpath;
#endif /* CNID_DB */
#ifdef FORCE_UIDGID
	char				*v_forceuid;
	char				*v_forcegid;
#endif /* FORCE_UIDGID */
};

#ifdef NO_LARGE_VOL_SUPPORT
typedef u_int32_t VolSpace;
#else /* NO_LARGE_VOL_SUPPORT */
typedef u_int64_t VolSpace;
#endif /* NO_LARGE_VOL_SUPPORT */

#define AFPVOL_OPEN	(1<<0)
#define AFPVOL_DT	(1<<1)

#define AFPVOL_GVSMASK	(7<<2)
#define AFPVOL_NONE	(0<<2)
#define AFPVOL_AFSGVS	(1<<2)
#define AFPVOL_USTATFS	(2<<2)
#define AFPVOL_UQUOTA	(4<<2)

/* flags that alter volume behaviour. */
#define AFPVOL_A2VOL     (1 << 5)   /* prodos volume */
#define AFPVOL_CRLF      (1 << 6)   /* cr/lf translation */
#define AFPVOL_NOADOUBLE (1 << 7)   /* don't create .AppleDouble by default */
#define AFPVOL_RO        (1 << 8)   /* read-only volume */
#define AFPVOL_MSWINDOWS (1 << 9)   /* deal with ms-windows yuckiness.
				       this is going away. */
#define AFPVOL_NOHEX     (1 << 10)  /* don't do :hex translation */
#define AFPVOL_USEDOTS   (1 << 11)  /* use real dots */
#define AFPVOL_LIMITSIZE (1 << 12)  /* limit size for older macs */
#define AFPVOL_MAPASCII  (1 << 13)  /* map the ascii range as well */
#define AFPVOL_DROPBOX   (1 << 14)  /* dropkludge dropbox support */

/* FPGetSrvrParms options */
#define AFPSRVR_CONFIGINFO     (1 << 0)
#define AFPSRVR_PASSWD         (1 << 7)

/* handle casefolding */
#define AFPVOL_MTOUUPPER       (1 << 0) 
#define AFPVOL_MTOULOWER       (1 << 1) 
#define AFPVOL_UTOMUPPER       (1 << 2) 
#define AFPVOL_UTOMLOWER       (1 << 3) 
#define AFPVOL_UMLOWER         (AFPVOL_MTOULOWER | AFPVOL_UTOMLOWER)
#define AFPVOL_UMUPPER         (AFPVOL_MTOUUPPER | AFPVOL_UTOMUPPER)
#define AFPVOL_UUPPERMLOWER    (AFPVOL_MTOUUPPER | AFPVOL_UTOMLOWER)
#define AFPVOL_ULOWERMUPPER    (AFPVOL_MTOULOWER | AFPVOL_UTOMUPPER)
 
#define MSWINDOWS_BADCHARS "\\/<>*?|\""
#define MSWINDOWS_CODEPAGE "maccode.iso8859-1"

#define AFPVOLSIG_FLAT          0x0001 /* flat fs */
#define AFPVOLSIG_FIX	        0x0002 /* fixed ids */
#define AFPVOLSIG_VAR           0x0003 /* variable ids */
#define AFPVOLSIG_DEFAULT       AFPVOLSIG_FIX

/* volume attributes */
#define VOLPBIT_ATTR_RO           (1 << 0)
#define VOLPBIT_ATTR_PASSWD       (1 << 1)
#define VOLPBIT_ATTR_FILEID       (1 << 2)
#define VOLPBIT_ATTR_CATSEARCH    (1 << 3)
#define VOLPBIT_ATTR_BLANKACCESS  (1 << 4)

#define VOLPBIT_ATTR	0
#define VOLPBIT_SIG	1
#define VOLPBIT_CDATE	2
#define VOLPBIT_MDATE	3
#define VOLPBIT_BDATE	4
#define VOLPBIT_VID	5
#define VOLPBIT_BFREE	6
#define VOLPBIT_BTOTAL	7
#define VOLPBIT_NAME	8
/* handle > 4GB volumes */
#define VOLPBIT_XBFREE  9
#define VOLPBIT_XBTOTAL 10
#define VOLPBIT_BSIZE   11        /* block size */


#define vol_noadouble(vol) (((vol)->v_flags & AFPVOL_NOADOUBLE) ? \
			    ADFLAGS_NOADOUBLE : 0)

extern struct vol	*getvolbyvid __P((const u_int16_t));
extern int              ustatfs_getvolspace __P((const struct vol *,
						 VolSpace *, VolSpace *,
						 u_int32_t *));
extern int              codepage_init __P((struct vol *, const int, 
					   const int));
extern int              codepage_read __P((struct vol *, const char *));
extern union codepage_val codepage_find __P(());
extern void             setvoltime __P((AFPObj *, struct vol *));

/* FP functions */
extern int	afp_openvol      __P((AFPObj *, char *, int, char *, int *));
extern int	afp_getvolparams __P((AFPObj *, char *, int, char *, int *));
extern int	afp_setvolparams __P((AFPObj *, char *, int, char *, int *));
extern int	afp_getsrvrparms __P((AFPObj *, char *, int, char *, int *));
extern int	afp_closevol     __P((AFPObj *, char *, int, char *, int *));

#endif
