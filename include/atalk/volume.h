/*
 * $Id: volume.h,v 1.16 2010/03/31 09:47:32 franklahm Exp $
 *
 * Copyright (c) 1990,1994 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifndef ATALK_VOLUME_H
#define ATALK_VOLUME_H 1

#include <sys/cdefs.h>
#include <sys/types.h>
#include <netatalk/endian.h>

#include <atalk/unicode.h>
#include <atalk/cnid.h>
#include <atalk/hash.h>
#include <atalk/vfs.h>

#define AFPVOL_U8MNAMELEN   255 /* AFP3 sepc */
#define AFPVOL_MACNAMELEN    27 /* AFP2 spec */

typedef u_int64_t VolSpace;

struct vol {
    struct vol      *v_next;
    u_int16_t       v_vid;
    int             v_flags;
    char            *v_path;
    struct dir      *v_root;
    time_t          v_mtime;

    charset_t       v_volcharset;
    charset_t       v_maccharset;
    u_int16_t       v_mtou_flags;    /* flags for convert_charset in mtoupath */
    u_int16_t       v_utom_flags;
    long            v_kTextEncoding; /* mac charset encoding in network order
                                      * FIXME: should be a u_int32_t ? */
    size_t          max_filename;
    char            *v_veto;
    int             v_adouble;    /* adouble format: v1, v2, sfm ... */
    int             v_ad_options; /* adouble option NODEV, NOCACHE, etc.. */
    char            *(*ad_path)(const char *, int);
    struct _cnid_db *v_cdb;
    char            v_stamp[ADEDLEN_PRIVSYN];
    VolSpace        v_limitsize; /* Size limit, if any, in MiB */
    mode_t          v_umask;
    mode_t          v_perm;  /* default permission value OR with requested perm*/
    mode_t          v_dperm; /* default directories permission value OR with requested perm*/
    mode_t          v_fperm; /* default files permission value OR with requested perm*/
    ucs2_t          *v_u8mname;     /* converted to utf8-mac in ucs2 */
    ucs2_t          *v_macname;     /* mangled to legacy longname in ucs2 */
    ucs2_t          *v_name;        /* either v_u8mname or v_macname */

    /* get/set volparams */
    time_t          v_ctime;  /* volume creation date, not unix ctime */
    dev_t           v_dev;    /* Unix volume device, Set but not used */

    /* adouble VFS indirection */
    struct vfs_ops  *vfs;   /* pointer to vfs_master_funcs for chaining */
    const struct vfs_ops *vfs_modules[4];
    int             v_vfs_ea;       /* The AFPVOL_EA_xx flag */

    /* misc */
    char            *v_gvs;
    void            *v_nfsclient;
    int             v_nfs;
    uintmax_t       v_tm_used;  /* used bytes on a TM volume */
    uintmax_t       v_written;  /* amount of data written in afp_write, reset every time a FCE_TM_SIZE event is sent */
    
    /* only when opening/closing volumes or in error */
    int             v_casefold;
    char            *v_localname;   /* as defined in AppleVolumes.default */
    char            *v_volcodepage;
    char            *v_maccodepage;
    char            *v_password;
    char            *v_cnidscheme;
    char            *v_dbpath;
    char            *v_cnidserver;
    char            *v_cnidport;
    int             v_hide;       /* new volume wait until old volume is closed */
    int             v_new;        /* volume deleted but there's a new one with the same name */
    int             v_deleted;    /* volume open but deleted in new config file */
    char            *v_root_preexec;
    char            *v_preexec;
    char            *v_root_postexec;
    char            *v_postexec;
    int             v_root_preexec_close;
    int             v_preexec_close;
    char            *v_uuid;    /* For TimeMachine zeroconf record */
#ifdef FORCE_UIDGID
    char            *v_forceuid;
    char            *v_forcegid;
#endif
#ifdef __svr4__
    int         v_qfd;
#endif /*__svr4__*/
};

#define AFPVOL_OPEN (1<<0)

/* flags  for AFS and quota 0xxx0 */
#define AFPVOL_GVSMASK  (7<<2)
#define AFPVOL_NONE (0<<2)
#define AFPVOL_AFSGVS   (1<<2)
#define AFPVOL_USTATFS  (2<<2)
#define AFPVOL_UQUOTA   (4<<2)

/*
  Flags that alter volume behaviour.
  Keep in sync with libatalk/util/volinfo.c
*/
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
#define AFPVOL_NOFILEID  (1 << 15)  /* don't advertise createid resolveid and deleteid calls */
#define AFPVOL_NOSTAT    (1 << 16)  /* advertise the volume even if we can't stat() it
                                     * maybe because it will be mounted later in preexec */
#define AFPVOL_UNIX_PRIV (1 << 17)  /* support unix privileges */
#define AFPVOL_NODEV     (1 << 18)  /* always use 0 for device number in cnid calls
                                     * help if device number is notconsistent across reboot
                                     * NOTE symlink to a different device will return an ACCESS error
                                     */
#define AFPVOL_CASEINSEN (1 << 19)  /* volume is case insensitive */
#define AFPVOL_EILSEQ    (1 << 20)  /* encode illegal sequence 'asis' UCS2, ex "\217-", which is not
                                       a valid SHIFT-JIS char, is encoded  as U\217 -*/

#define AFPVOL_CACHE     (1 << 21)   /* Use adouble v2 CNID caching. Default: yes */
#define AFPVOL_INV_DOTS  (1 << 22)   /* dots files are invisible */
#define AFPVOL_TM        (1 << 23)   /* Supports TimeMachine */
#define AFPVOL_ACLS      (1 << 24)   /* Volume supports ACLS */
#define AFPVOL_SEARCHDB  (1 << 25)   /* Use fast CNID db search instead of filesystem */
#define AFPVOL_NONETIDS  (1 << 26)   /* signal the client it shall do privelege mapping */

/* Extended Attributes vfs indirection  */
#define AFPVOL_EA_NONE           0   /* No EAs */
#define AFPVOL_EA_AUTO           1   /* try sys, fallback to ad (default) */
#define AFPVOL_EA_SYS            2   /* Store them in native EAs */
#define AFPVOL_EA_AD             3   /* Store them in adouble files */

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

#define MSWINDOWS_BADCHARS ":\t\\/<>*?|\""

int wincheck(const struct vol *vol, const char *path);

#define AFPVOLSIG_FLAT          0x0001 /* flat fs */
#define AFPVOLSIG_FIX           0x0002 /* fixed ids */
#define AFPVOLSIG_VAR           0x0003 /* variable ids */
#define AFPVOLSIG_DEFAULT       AFPVOLSIG_FIX

/* volume attributes */
#define VOLPBIT_ATTR_RO           (1 << 0)
#define VOLPBIT_ATTR_PASSWD       (1 << 1)
#define VOLPBIT_ATTR_FILEID       (1 << 2)
#define VOLPBIT_ATTR_CATSEARCH    (1 << 3)
#define VOLPBIT_ATTR_BLANKACCESS  (1 << 4)
#define VOLPBIT_ATTR_UNIXPRIV     (1 << 5)
#define VOLPBIT_ATTR_UTF8         (1 << 6)
#define VOLPBIT_ATTR_NONETIDS     (1 << 7)
#define VOLPBIT_ATTR_EXT_ATTRS    (1 << 10)
#define VOLPBIT_ATTR_ACLS         (1 << 11)
#define VOLPBIT_ATTR_TM           (1 << 13)

#define VOLPBIT_ATTR    0
#define VOLPBIT_SIG 1
#define VOLPBIT_CDATE   2
#define VOLPBIT_MDATE   3
#define VOLPBIT_BDATE   4
#define VOLPBIT_VID 5
#define VOLPBIT_BFREE   6
#define VOLPBIT_BTOTAL  7
#define VOLPBIT_NAME    8
/* handle > 4GB volumes */
#define VOLPBIT_XBFREE  9
#define VOLPBIT_XBTOTAL 10
#define VOLPBIT_BSIZE   11        /* block size */

#define utf8_encoding() (afp_version >= 30)

#define vol_noadouble(vol) (((vol)->v_flags & AFPVOL_NOADOUBLE) ? 1 : 0)
#define vol_nodev(vol) (((vol)->v_flags & AFPVOL_NODEV) ? 1 : 0)
#define vol_unix_priv(vol) (afp_version >= 30 && ((vol)->v_flags & AFPVOL_UNIX_PRIV))
#define vol_inv_dots(vol) (((vol)->v_flags & AFPVOL_INV_DOTS) ? 1 : 0)


#endif
