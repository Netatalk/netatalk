/*
 * $Id: unix.h,v 1.14 2003-12-15 04:49:56 srittau Exp $
 */

#ifndef AFPD_UNIX_H
#define AFPD_UNIX_H

#ifdef HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif /* HAVE_SYS_CDEFS_H */
#include <netatalk/endian.h>
#include "volume.h"

#if defined( sun ) && !defined( __svr4__ )
#ifdef i386
typedef int	mode_t;
#endif /*i386*/
#endif /*sun __svr4__*/

#if defined(HAVE_SYS_VFS_H) || defined( sun ) || defined( ibm032 ) 
#include <sys/vfs.h>
#endif /* HAVE_SYS_VFS_H || sun || ibm032 */

#if defined(_IBMR2) || defined(HAVE_STATFS_H) 
#include <sys/statfs.h>
/* this might not be right. */
#define f_mntfromname f_fname
#endif /* _IBMR2 || HAVE_STATFS_H */

#if defined(TRU64)
#define f_frsize f_fsize
#else /* TRU64 */
#if defined(HAVE_SYS_STATVFS_H) || defined(__svr4__)
#include <sys/statvfs.h>
#define statfs statvfs
#else /* HAVE_SYS_STATVFS || __svr4__ */
#define	f_frsize f_bsize
#endif /* USE_STATVFS_H */
#endif /* TRU64 */

#if defined(__svr4__) || defined(HAVE_SYS_MNTTAB_H)
#include <sys/mnttab.h>
#endif /* __svr4__ || HAVE_SYS_MNTTAB_H */

#if defined(HAVE_SYS_MOUNT_H) || defined(BSD4_4) || \
    defined(linux) || defined(ultrix)
#include <sys/mount.h>
#endif /* HAVE_SYS_MOUNT_H || BSD4_4 || linux || ultrix */

#if defined(linux) || defined(HAVE_MNTENT_H)
#include <mntent.h>
#endif /* linux || HAVE_MNTENT_H */


#ifndef NO_QUOTA_SUPPORT

#if !(defined(__svr4__) || defined(HAVE_DQB_BTIMELIMIT))
#define dqb_btimelimit  dqb_btime
#endif /* ! __svr4__ || HAVE_DQB_BTIMELIMIT */

#if defined(linux) || defined(ultrix) || defined(HAVE_QUOTA_H)
#include <sys/quota.h>
#endif /* linux || ultrix || HAVE_QUOTA_H */

#ifdef __svr4__ 
#include <sys/fs/ufs_quota.h>
#endif /* __svr4__ */

#ifdef BSD4_4
#include <ufs/ufs/quota.h>
#endif /* BSD4_4 */

#ifdef HAVE_UFS_QUOTA_H
#include <ufs/quota.h>
#endif /* HAVE_UFS_QUOTA_H */

#ifdef _IBMR2
#include <jfs/quota.h>
#endif /* _IBMR2 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "directory.h"

extern int getnfsquota __P((const struct vol *, const int, const u_int32_t,
                                struct dqblk *));

extern int uquota_getvolspace __P((const struct vol *, VolSpace *, VolSpace *,
                                       const u_int32_t));
#endif /* NO_QUOTA_SUPPORT */

extern struct afp_options default_options;

extern int gmem            __P((const gid_t));
extern int setdeskmode      __P((const mode_t));
extern int setdirunixmode   __P((const mode_t, const int, const int));
extern int setdirmode       __P((const mode_t, const int, const int));
extern int setdeskowner     __P((const uid_t, const gid_t));
extern int setdirowner      __P((const uid_t, const gid_t, const int));
extern int setfilmode       __P((char *, mode_t , struct stat *));
extern int setfilemode      __P((struct path*, const mode_t));
extern int unix_rename      __P((const char *oldpath, const char *newpath));

extern void accessmode      __P((char *, struct maccess *, struct dir *, struct stat *));

#ifdef AFS	
    #define accessmode afsmode
#endif 

#endif /* UNIX_H */
