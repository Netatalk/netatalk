/*
 * $Id: volinfo.h,v 1.2 2005-04-28 20:49:51 bfernhomberg Exp $
 */

#ifndef _ATALK_VOLINFO_H
#define _ATALK_VOLINFO_H 1

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
                                     * NOTE symlink to a different device will return an ACCESS error
                                     */
/* handle casefolding */
#define AFPVOL_MTOUUPPER       (1 << 0)
#define AFPVOL_MTOULOWER       (1 << 1)
#define AFPVOL_UTOMUPPER       (1 << 2)
#define AFPVOL_UTOMLOWER       (1 << 3)
#define AFPVOL_UMLOWER         (AFPVOL_MTOULOWER | AFPVOL_UTOMLOWER)
#define AFPVOL_UMUPPER         (AFPVOL_MTOUUPPER | AFPVOL_UTOMUPPER)
#define AFPVOL_UUPPERMLOWER    (AFPVOL_MTOUUPPER | AFPVOL_UTOMLOWER)
#define AFPVOL_ULOWERMUPPER    (AFPVOL_MTOULOWER | AFPVOL_UTOMUPPER)


#endif
