/*
 * Copyright (c) 1990,1994 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifndef ATALK_VOLUME_H
#define ATALK_VOLUME_H 1

#include <stdint.h>
#include <sys/types.h>

#include <atalk/cnid.h>
#include <atalk/globals.h>
#include <atalk/hash.h>
#include <atalk/unicode.h>
#include <atalk/vfs.h>

/* AFP3 spec */
#define AFPVOL_U8MNAMELEN   255
/* AFP2 spec */
#define AFPVOL_MACNAMELEN    27

typedef uint64_t VolSpace;

/* This should belong in a file.h */
struct extmap {
    char		*em_ext;
    char		em_creator[4];
    char		em_type[4];
};

struct vol {
    struct vol      *v_next;
    AFPObj          *v_obj;
    uint16_t        v_vid;
    int             v_flags;
    char            *v_path;
    struct dir      *v_root;
    time_t          v_mtime;

    charset_t       v_volcharset;
    charset_t       v_maccharset;
    /* flags for convert_charset in mtoupath */
    uint16_t        v_mtou_flags;
    uint16_t        v_utom_flags;
    /* mac charset encoding in network order */
    uint32_t        v_kTextEncoding;
    size_t          max_filename;
    char            *v_veto;
    /* adouble format: v1, v2, sfm ... */
    int             v_adouble;
    /* adouble option NODEV, NOCACHE, etc.. */
    int             v_ad_options;
    const char      *(*ad_path)(const char *, int);
    struct _cnid_db *v_cdb;
    char            v_stamp[ADEDLEN_PRIVSYN];
    /* Size limit, if any, in MiB */
    VolSpace        v_limitsize;
    mode_t          v_umask;
    /* default directories permission value OR with requested perm*/
    mode_t          v_dperm;
    /* default files permission value OR with requested perm*/
    mode_t          v_fperm;
    /* converted to utf8-mac in ucs2 */
    ucs2_t          *v_u8mname;
    /* mangled to legacy longname in ucs2 */
    ucs2_t          *v_macname;
    /* either v_u8mname or v_macname */
    ucs2_t          *v_name;

    /* get/set volparams */
    /* volume creation date, not unix ctime */
    time_t          v_ctime;
    /* Unix volume device, Set but not used */
    dev_t           v_dev;

    /* adouble VFS indirection */
    /* pointer to vfs_master_funcs for chaining */
    struct vfs_ops  *vfs;
    const struct vfs_ops *vfs_modules[4];
    /* The AFPVOL_EA_xx flag */
    int             v_vfs_ea;

    /* misc */
    char            *v_gvs;
    void            *v_nfsclient;
    int             v_nfs;
    /* used bytes on a TM volume */
    VolSpace        v_tm_used;
    /* time at which v_tm_used was calculated last */
    time_t          v_tm_cachetime;
    /* amount of data appended to files */
    VolSpace        v_appended;

    /* only when opening/closing volumes or in error */
    int             v_casefold;
    /* as defined in afpc.conf */
    char            *v_configname;
    /* as defined in afp.conf but with vars expanded */
    char            *v_localname;
    char            *v_volcodepage;
    char            *v_maccodepage;
    char            *v_password;
    char            *v_cnidscheme;
    char            *v_dbpath;
    char            *v_cnidserver;
    char            *v_cnidport;
#if 0
    /* new volume wait until old volume is closed */
    int             v_hide;
    /* volume deleted but there's a new one with the same name */
    int             v_new;
#endif
    /* volume open but deleted in new config file */
    int             v_deleted;
#if 0
    char            *v_root_preexec;
#endif
    char            *v_preexec;
#if 0
    char            *v_root_postexec;
#endif
    char            *v_postexec;
#if 0
    int             v_root_preexec_close;
#endif
    int             v_preexec_close;
    /* For TimeMachine zeroconf record */
    char            *v_uuid;
    int             v_qfd;
    /* AFP attributes that shall be ignored */
    uint32_t        v_ignattr;
};

/* load_volumes() flags */
typedef enum {
    LV_DEFAULT = 0,
    /* Skip access checks */
    LV_ALL = 1,
    /* Reload even if unchanged */
    LV_FORCE = 2
} lv_flags_t;

/* volume flags */
#define AFPVOL_OPEN (1<<0)

/* flags for quota 0xxx0 */
#define AFPVOL_GVSMASK  (7<<2)
#define AFPVOL_NONE     (0<<2)
#if 0
/* Previously used for Andrew File System */
#define AFPVOL_AFSGVS   (1<<2)
#endif
#define AFPVOL_USTATFS  (1<<3)
#define AFPVOL_UQUOTA   (1<<4)

/* no adouble:v2 to adouble:ea conversion */
#define AFPVOL_NOV2TOEACONV (1 << 5)
/* Index volume for Spotlight searches */
#define AFPVOL_SPOTLIGHT (1 << 6)
/* Store Samba compatible xattrs (append 0 byte) */
#define AFPVOL_EA_SAMBA  (1 << 7)
/* read-only volume */
#define AFPVOL_RO        (1 << 8)
/* try to preserve ACLs */
#define AFPVOL_CHMOD_PRESERVE_ACL (1 << 9)
/* try to preserve ACLs */
#define AFPVOL_CHMOD_IGNORE (1 << 10)
/* write metadata xattr as root on sticky dirs */
#define AFPVOL_FORCE_STICKY_XATTR (1 << 11)
/* limit size for older macs */
#define AFPVOL_LIMITSIZE (1 << 12)
/* prodos volume */
#define AFPVOL_A2VOL     (1 << 13)
/* advertise the volume even if we can't stat() it
 * maybe because it will be mounted later in preexec */
#define AFPVOL_NOSTAT    (1 << 16)
/* support unix privileges */
#define AFPVOL_UNIX_PRIV (1 << 17)
/* always use 0 for device number in cnid calls
 * help if device number is notconsistent across reboot
 * NOTE symlink to a different device will return an ACCESS error
 */
#define AFPVOL_NODEV     (1 << 18)
/* encode illegal sequence 'asis' UCS2, ex "\217-", which is not
 * a valid SHIFT-JIS char, is encoded as U\217 -*/
#define AFPVOL_EILSEQ    (1 << 20)
/* dots files are invisible */
#define AFPVOL_INV_DOTS  (1 << 22)
/* Supports TimeMachine */
#define AFPVOL_TM        (1 << 23)
/* Volume supports ACLS */
#define AFPVOL_ACLS      (1 << 24)
/* Use fast CNID db search instead of filesystem */
#define AFPVOL_SEARCHDB  (1 << 25)
/* signal the client it shall do privelege mapping */
#define AFPVOL_NONETIDS  (1 << 26)
/* follow symlinks on the server, default is not to */
#define AFPVOL_FOLLOWSYM (1 << 27)
/* delete veto files and dirs */
#define AFPVOL_DELVETO   (1 << 28)

/* Extended Attributes vfs indirection */
/* No EAs */
#define AFPVOL_EA_NONE           0
/* try sys, fallback to ad (default) */
#define AFPVOL_EA_AUTO           1
/* Store them in native EAs */
#define AFPVOL_EA_SYS            2
/* Store them in adouble files */
#define AFPVOL_EA_AD             3

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
#define AFPVOL_CASESENS        (1 << 4)

/* flat fs */
#define AFPVOLSIG_FLAT          0x0001
/* fixed ids */
#define AFPVOLSIG_FIX           0x0002
/* variable ids */
#define AFPVOLSIG_VAR           0x0003
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
#define VOLPBIT_ATTR_PRIVPARENT   (1 << 8)
#define VOLPBIT_ATTR_NOTFILEXCHG  (1 << 9)
#define VOLPBIT_ATTR_EXT_ATTRS    (1 << 10)
#define VOLPBIT_ATTR_ACLS         (1 << 11)
#define VOLPBIT_ATTR_CASESENS     (1 << 12)
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
/* block size */
#define VOLPBIT_BSIZE   11

#define utf8_encoding(obj) ((obj)->afp_version >= 30)

#define vol_nodev(vol) (((vol)->v_flags & AFPVOL_NODEV) ? 1 : 0)
#define vol_unix_priv(vol) ((vol)->v_obj->afp_version >= 30 && ((vol)->v_flags & AFPVOL_UNIX_PRIV))
#define vol_inv_dots(vol) (((vol)->v_flags & AFPVOL_INV_DOTS) ? 1 : 0)
#define vol_syml_opt(vol) (((vol)->v_flags & AFPVOL_FOLLOWSYM) ? 0 : O_NOFOLLOW)
#define vol_chmod_opt(vol) (((vol)->v_flags & AFPVOL_CHMOD_PRESERVE_ACL) ? O_NETATALK_ACL : \
                            ((vol)->v_flags & AFPVOL_CHMOD_IGNORE) ? O_IGNORE : 0)

#endif
