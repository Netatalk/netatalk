#ifndef AFPD_UNIX_H
#define AFPD_UNIX_H

#include <arpa/inet.h>

#include "config.h"
#include "volume.h"

#if defined( sun ) && !defined( __svr4__ )
#ifdef i386
typedef int	mode_t;
#endif /*i386*/
#endif /*sun __svr4__*/

#if defined(HAVE_SYS_VFS_H) || defined( sun )
#include <sys/vfs.h>
#endif /* HAVE_SYS_VFS_H || sun */

#if defined(_IBMR2) || defined(HAVE_STATFS_H)
#include <sys/statfs.h>
/* this might not be right. */
#define f_mntfromname f_fname
#endif /* _IBMR2 || HAVE_STATFS_H */

/* temp fix, was: defined(HAVE_SYS_STATVFS) || defined(__svr4__) */
#if defined(__svr4__) || (defined(__NetBSD__) && (__NetBSD_Version__ >= 200040000))
#include <sys/statvfs.h>
#define statfs statvfs
#else /* HAVE_SYS_STATVFS || __svr4__ */
#define	f_frsize f_bsize
#endif /* USE_STATVFS_H */

#if defined(__svr4__) || defined(HAVE_SYS_MNTTAB_H)
#include <sys/mnttab.h>
#endif /* __svr4__ || HAVE_SYS_MNTTAB_H */

#if defined(__DragonFly__)
#define dqblk ufs_dqblk
#endif

#include <sys/mount.h>

#if defined(linux) || defined(HAVE_MNTENT_H)
#include <mntent.h>
#endif /* linux || HAVE_MNTENT_H */

extern struct afp_options default_options;

extern int setdirunixmode   (const struct vol *, char *, mode_t);
extern int setdirmode       (const struct vol *, const char *, mode_t);
extern int setdirowner      (const struct vol *, const char *, const uid_t, const gid_t);
extern int setfilunixmode   (const struct vol *, struct path*, const mode_t);
extern int setfilowner      (const struct vol *, const uid_t, const gid_t, struct path*);
extern void accessmode      (const AFPObj *obj, const struct vol *, char *, struct maccess *, struct dir *, struct stat *);

#ifdef AFS	
    #define accessmode afsmode
#endif

#endif /* UNIX_H */
