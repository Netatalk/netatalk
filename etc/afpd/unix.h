/*
 * $Id: unix.h,v 1.6 2001-05-22 19:13:36 rufustfirefly Exp $
 */

#ifndef AFPD_UNIX_H
#define AFPD_UNIX_H

#include <sys/cdefs.h>
#include <netatalk/endian.h>
#include "volume.h"

#if defined( sun ) && !defined( __svr4__ )
#ifdef i386
typedef int	mode_t;
#endif /*i386*/
#endif /*sun __svr4__*/


/* some GLIBC/old-libc-isms */
#if defined(__GNU_LIBRARY__) 
#if __GNU_LIBRARY__ < 6
#define HAVE_SYS_VFS_H
#else
#define HAVE_STATFS_H
#endif
#endif

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
#ifndef NEED_QUOTACTL_WRAPPER
#include <sys/quota.h>
#else /* ! NEED_QUOTACTL_WRAPPER */
#include <asm/types.h>
#include <asm/unistd.h>
#include <linux/quota.h>
#endif /* ! NEED_QUOTACTL_WRAPPER */
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

extern int getnfsquota __P((const struct vol *, const int, const u_int32_t,
		            struct dqblk *));

extern int uquota_getvolspace __P((const struct vol *, VolSpace *, VolSpace *,
			           const u_int32_t));
#endif /* NO_QUOTA_SUPPORT */

extern int gmem         __P((const gid_t));
extern int setdeskmode  __P((const mode_t));
extern int setdirmode   __P((const mode_t, const int, const int));
extern int setdeskowner __P((const uid_t, const gid_t));
extern int setdirowner  __P((const uid_t, const gid_t, const int));

#endif /* UNIX_H */
