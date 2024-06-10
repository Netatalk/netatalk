#ifndef AFPD_UNIX_H
#define AFPD_UNIX_H

#ifdef HAVE_SYS_CDEFS_H
#include <sys/types.h>
#endif /* HAVE_SYS_CDEFS_H */
#include <netatalk/endian.h>
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

#if defined(HAVE_SYS_MOUNT_H) || defined(BSD4_4) || defined(__linux__)
#include <sys/mount.h>
#endif /* HAVE_SYS_MOUNT_H || BSD4_4 || __linux__ */

#if defined(__linux__) || defined(HAVE_MNTENT_H)
#include <mntent.h>
#endif /* linux || HAVE_MNTENT_H */

#ifndef NO_QUOTA_SUPPORT
#if !defined(HAVE_LIBQUOTA)

#if !(defined(__svr4__) || defined(HAVE_DQB_BTIMELIMIT))
#define dqb_btimelimit  dqb_btime
#endif /* ! __svr4__ || HAVE_DQB_BTIMELIMIT */

#if defined(__linux__) || defined(HAVE_QUOTA_H)
#ifndef NEED_QUOTACTL_WRAPPER
/*#include <sys/quota.h>*/
/*long quotactl (int, const char *, unsigned int, caddr_t); */
/* extern long quotactl (int, const char *, long, caddr_t); */

#else /* ! NEED_QUOTACTL_WRAPPER */
#include <asm/types.h>
#include <asm/unistd.h>
#include <linux/quota.h>
#endif /* ! NEED_QUOTACTL_WRAPPER */
#endif /* linux || HAVE_QUOTA_H */

#ifdef __svr4__
#include <sys/fs/ufs_quota.h>
#endif /* __svr4__ */

#ifdef BSD4_4
#if defined(__DragonFly__)
#include <vfs/ufs/quota.h>
#elif defined(__NetBSD__)
#include <ufs/ufs/quota.h>
#include <ufs/ufs/quota1.h>
#include <ufs/ufs/quota2.h>
#else /* DragonFly */
#include <ufs/ufs/quota.h>
#endif /* DragonFly */
#endif /* BSD4_4 */

#ifdef HAVE_UFS_QUOTA_H
#include <ufs/quota.h>
#endif /* HAVE_UFS_QUOTA_H */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "directory.h"


#if defined (__linux__)

#define MAXQUOTAS 2

/* definitions from sys/quota.h */
#define USRQUOTA  0             /* element used for user quotas */
#define GRPQUOTA  1             /* element used for group quotas */

/*
 * Command definitions for the 'quotactl' system call.
 * The commands are broken into a main command defined below
 * and a subcommand that is used to convey the type of
 * quota that is being manipulated (see above).
 */
#define SUBCMDMASK  0x00ff
#define SUBCMDSHIFT 8
#define QCMD(cmd, type)  (((cmd) << SUBCMDSHIFT) | ((type) & SUBCMDMASK))

/* declare an internal version of the quota block struct */
typedef uint64_t qsize_t;	/* Type in which we store size limitations */
typedef uint32_t qid_t;	/* Type in which we store ids in memory */

struct dqblk {
  qsize_t bsize;
  qsize_t dqb_ihardlimit;   /* absolute limit on allocated inodes */
  qsize_t dqb_isoftlimit;   /* preferred inode limit */
  qsize_t dqb_curinodes;    /* current # allocated inodes */
  qsize_t dqb_bhardlimit;   /* absolute limit on disk blks alloc */
  qsize_t dqb_bsoftlimit;   /* preferred limit on disk blks */
  qsize_t dqb_curblocks;    /* current block count */
  time_t  dqb_btime;        /* time limit for excessive disk use */
  time_t  dqb_itime;        /* time limit for excessive inode use */
};

/* API v1 command definitions */
#define Q_V1_GETQUOTA  0x0300
#define Q_V1_SYNC      0x0600
#define Q_V1_SETQLIM   0x0700
#define Q_V1_GETSTATS  0x0800
/* API v2 command definitions */
#define Q_V2_SYNC      0x0600
#define Q_V2_SETQLIM   0x0700
#define Q_V2_GETQUOTA  0x0D00
#define Q_V2_GETSTATS  0x1100
/* proc API command definitions */
#define Q_V3_SYNC      0x800001
#define Q_V3_GETQUOTA  0x800007
#define Q_V3_SETQUOTA  0x800008

/* Interface versions */
#define IFACE_UNSET 0
#define IFACE_VFSOLD 1
#define IFACE_VFSV0 2
#define IFACE_GENERIC 3

#define DEV_QBSIZE 1024

struct dqblk_v3 {
  uint64_t dqb_bhardlimit;
  uint64_t dqb_bsoftlimit;
  uint64_t dqb_curspace;
  uint64_t dqb_ihardlimit;
  uint64_t dqb_isoftlimit;
  uint64_t dqb_curinodes;
  uint64_t dqb_btime;
  uint64_t dqb_itime;
  uint32_t dqb_valid;
};

struct dqblk_v2 {
  unsigned int dqb_ihardlimit;
  unsigned int dqb_isoftlimit;
  unsigned int dqb_curinodes;
  unsigned int dqb_bhardlimit;
  unsigned int dqb_bsoftlimit;
  qsize_t dqb_curspace;
  time_t dqb_btime;
  time_t dqb_itime;
};

struct dqstats_v2 {
  uint32_t lookups;
  uint32_t drops;
  uint32_t reads;
  uint32_t writes;
  uint32_t cache_hits;
  uint32_t allocated_dquots;
  uint32_t free_dquots;
  uint32_t syncs;
  uint32_t version;
};

struct dqblk_v1 {
  uint32_t dqb_bhardlimit;
  uint32_t dqb_bsoftlimit;
  uint32_t dqb_curblocks;
  uint32_t dqb_ihardlimit;
  uint32_t dqb_isoftlimit;
  uint32_t dqb_curinodes;
  time_t dqb_btime;
  time_t dqb_itime;
};

extern long quotactl (unsigned int, const char *, int, caddr_t);



#endif /* __linux__ */

extern int getnfsquota (struct vol *, const int, const uint32_t,
                                struct dqblk *);

#endif /* ! HAVE_LIBQUOTA */
extern int uquota_getvolspace (struct vol *, VolSpace *, VolSpace *,
                                       const uint32_t);
#endif /* NO_QUOTA_SUPPORT */

extern struct afp_options default_options;

extern int gmem            (const gid_t);
extern int setdeskmode      (const mode_t);
extern int setdirunixmode   (const struct vol *, const char *, mode_t);
extern int setdirmode       (const struct vol *, const char *, mode_t);
extern int setdeskowner     (const uid_t, const gid_t);
extern int setdirowner      (const struct vol *, const char *, const uid_t, const gid_t);
extern int setfilunixmode   (const struct vol *, struct path*, const mode_t);
extern int setfilowner      (const struct vol *, const uid_t, const gid_t, struct path*);
extern void accessmode      (const struct vol *, char *, struct maccess *, struct dir *, struct stat *);

#endif /* UNIX_H */
