/*
 * $Id: unix.h,v 1.4 2001-05-11 12:17:45 rufustfirefly Exp $
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
#define USE_VFS_H
#else
#define USE_STATFS_H
#endif
#endif

#if defined(USE_VFS_H) || defined( sun ) || defined( ibm032 ) 
#include <sys/vfs.h>
#endif

#if defined(_IBMR2) || defined(USE_STATFS_H) 
#include <sys/statfs.h>
/* this might not be right. */
#define f_mntfromname f_fname
#endif

#if defined(HAVE_SYS_STATVFS_H) || defined(__svr4__)
#include <sys/statvfs.h>
#define statfs statvfs
#else
#if defined(TRU64)
#define f_frsize f_fsize
#else /* TRU64 */
#define	f_frsize f_bsize
#endif /* TRU64 */
#endif /* USE_STATVFS_H */

#if defined(__svr4__) || defined(HAVE_SYS_MNTTAB_H)
#include <sys/mnttab.h>
#endif

#if defined(HAVE_MOUNT_H) || defined(BSD4_4) || \
    defined(linux) || defined(ultrix)
#include <sys/mount.h>
#endif

#if defined(linux) || defined(HAVE_MNTENT_H)
#include <mntent.h>
#endif


#ifndef NO_QUOTA_SUPPORT

#if !(defined(__svr4__) || defined(HAVE_DQB_BTIMELIMIT))
#define dqb_btimelimit  dqb_btime
#endif

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
#endif

#ifdef HAVE_UFS_QUOTA_H
#include <ufs/quota.h>
#endif /* HAVE_UFS_QUOTA_H */

#ifdef _IBMR2
#include <jfs/quota.h>
#endif

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
