#ifndef AFPD_UNIX_H
#define AFPD_UNIX_H

#include <arpa/inet.h>

#include "config.h"
#include "volume.h"

#if defined(HAVE_SYS_VFS_H)
#include <sys/vfs.h>
#endif /* HAVE_SYS_VFS_H */

#if defined(HAVE_STATFS_H) 
#include <sys/statfs.h>
/* this might not be right. */
#define f_mntfromname f_fname
#endif /* HAVE_STATFS_H */

#if defined(__svr4__) || defined(__NetBSD__)
#include <sys/statvfs.h>
#define statfs statvfs
#else
#define	f_frsize f_bsize
#endif /* __svr4__ || __NetBSD__ */

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

#endif /* UNIX_H */
