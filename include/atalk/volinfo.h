/*
 * $Id: volinfo.h,v 1.7 2009-09-11 09:14:16 franklahm Exp $
 */

#ifndef _ATALK_VOLINFO_H
#define _ATALK_VOLINFO_H 1

#include <atalk/unicode.h>

/* FIXME: following duplicated from etc/afpd/volume.h  */

/* flags that alter volume behaviour. */
#define AFPVOL_A2VOL     (1 << 5)   /* prodos volume */
#define AFPVOL_CRLF      (1 << 6)   /* cr/lf translation */
#define AFPVOL_NOADOUBLE (1 << 7)   /* don't create .AppleDouble by default */
#define AFPVOL_RO        (1 << 8)   /* read-only volume */
#define AFPVOL_MSWINDOWS (1 << 9)   /* deal with ms-windows yuckiness. this is going away. */
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
                                     * NOTE symlink to a different device will return an ACCESS error */
#define AFPVOL_CASEINSEN (1 << 19)  /* volume is case insensitive */
#define AFPVOL_EILSEQ    (1 << 20)  /* encode illegal sequence 'asis' UCS2, ex "\217-", which is not 
                                       a valid SHIFT-JIS char, is encoded  as U\217 -*/
#define AFPVOL_CACHE     (1 << 21)   /* Use adouble v2 CNID caching, default don't use it */
#define AFPVOL_INV_DOTS  (1 << 22)   /* dots files are invisible */
#define AFPVOL_EXT_ATTRS (1 << 23)   /* Volume supports Extended Attributes */
#define AFPVOL_TM        (1 << 24)   /* Supports TimeMachine */
#define AFPVOL_ACLS      (1 << 25)   /* Volume supports ACLS */

/* handle casefolding */
#define AFPVOL_MTOUUPPER       (1 << 0)
#define AFPVOL_MTOULOWER       (1 << 1)
#define AFPVOL_UTOMUPPER       (1 << 2)
#define AFPVOL_UTOMLOWER       (1 << 3)
#define AFPVOL_UMLOWER         (AFPVOL_MTOULOWER | AFPVOL_UTOMLOWER)
#define AFPVOL_UMUPPER         (AFPVOL_MTOUUPPER | AFPVOL_UTOMUPPER)
#define AFPVOL_UUPPERMLOWER    (AFPVOL_MTOUUPPER | AFPVOL_UTOMLOWER)
#define AFPVOL_ULOWERMUPPER    (AFPVOL_MTOULOWER | AFPVOL_UTOMUPPER)

/* volinfo for shell utilities */
#define VOLINFODIR  ".AppleDesktop"
#define VOLINFOFILE ".volinfo"

struct volinfo {
    char                *v_name;
    char                *v_path;
    int                 v_flags;
    int                 v_casefold;
    char                *v_cnidscheme;
    char                *v_dbpath;
    char                *v_volcodepage;
    charset_t           v_volcharset;
    char                *v_maccodepage;
    charset_t           v_maccharset;
    int                 v_adouble;  /* default adouble format */
    int                 v_ad_options;
    char                *(*ad_path)(const char *, int);
    char                *v_dbd_host;
    int                 v_dbd_port;
};

extern int loadvolinfo(char *path, struct volinfo *vol);
extern int vol_load_charsets(struct volinfo *vol);

#endif
