/*
 * $Id: quota.c,v 1.5 2001-05-07 20:05:32 rufustfirefly Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/param.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>

#include <atalk/afp.h>

#include "auth.h"
#include "volume.h"
#include "unix.h"

#ifndef NO_QUOTA_SUPPORT
#ifdef NEED_QUOTACTL_WRAPPER
int quotactl(int cmd, const char *special, int id, caddr_t addr)
{
  return syscall(__NR_quotactl, cmd, special, id, addr);
}
#endif


#if defined(HAVE_SYS_MNTTAB_H) || defined(__svr4__)
/*
 * Return the mount point associated with the filesystem
 * on which "file" resides.  Returns NULL on failure.
 */
static char *
mountp( file, nfs)
    char	*file;
    int         *nfs;
{
    struct stat			sb;
    FILE 			*mtab;
    dev_t			devno;
    static struct mnttab	mnt;

    if ( stat( file, &sb ) < 0 ) {
	return( NULL );
    }
    devno = sb.st_dev;

    if (( mtab = fopen( "/etc/mnttab", "r" )) == NULL ) {
	return( NULL );
    }

    while ( getmntent( mtab, &mnt ) == 0 ) {
      /* local fs */
      if ( (stat( mnt.mnt_special, &sb ) == 0) && (devno == sb.st_rdev)) {
	fclose( mtab );
	return mnt.mnt_mountp;
      }
	  
      /* check for nfs. we probably should use 
       * strcmp(mnt.mnt_fstype, MNTTYPE_NFS), but that's not as fast. */
      if ((stat(mnt.mnt_mountp, &sb) == 0) && (devno == sb.st_dev) &&
	  strchr(mnt.mnt_special, ':')) {
	*nfs = 1;
	fclose( mtab );
	return mnt.mnt_special;
      }
    }

    fclose( mtab );
    return( NULL );
}

#else /* __svr4__ */
#ifdef ultrix
/*
 * Return the block-special device name associated with the filesystem
 * on which "file" resides.  Returns NULL on failure.
 */

static char *
special( file, nfs )
    char *file;
    int  *nfs;
{
    static struct fs_data	fsd;

    if ( getmnt(0, &fsd, 0, STAT_ONE, file ) < 0 ) {
	syslog(LOG_INFO, "special: getmnt %s: %m", file );
	return( NULL );
    }

    /* XXX: does this really detect an nfs mounted fs? */
    if (strchr(fsd.fd_req.devname, ':'))
      *nfs = 1;
    return( fsd.fd_req.devname );
}

#else /* ultrix */
#if defined(HAVE_MOUNT_H) || defined(BSD4_4) || defined(_IBMR2)

static char *
special( file, nfs )
    char	*file;
    int         *nfs;
{
    static struct statfs	sfs;

    if ( statfs( file, &sfs ) < 0 ) {
	return( NULL );
    }

    /* XXX: make sure this really detects an nfs mounted fs */
    if (strchr(sfs.f_mntfromname, ':'))
      *nfs = 1;
    return( sfs.f_mntfromname );
}

#else /* BSD4_4 */

static char *
special( file, nfs )
    char *file;
    int *nfs;
{
    struct stat		sb;
    FILE 		*mtab;
    dev_t		devno;
    struct mntent	*mnt;

    if ( stat( file, &sb ) < 0 ) {
	return( NULL );
    }
    devno = sb.st_dev;

    if (( mtab = setmntent( "/etc/mtab", "r" )) == NULL ) {
	return( NULL );
    }

    while (( mnt = getmntent( mtab )) != NULL ) {
      /* check for local fs */
      if ( (stat( mnt->mnt_fsname, &sb ) == 0) && devno == sb.st_rdev) {
	endmntent( mtab );
	return( mnt->mnt_fsname );
      }
      
      /* check for an nfs mount entry. the alternative is to use
       * strcmp(mnt->mnt_type, MNTTYPE_NFS) instead of the strchr. */
      if ((stat(mnt->mnt_dir, &sb) == 0) && (devno == sb.st_dev) &&
	  strchr(mnt->mnt_fsname, ':')) {
	*nfs = 1;
	endmntent( mtab );
	return( mnt->mnt_fsname );
      }
    }

    endmntent( mtab );
    return( NULL );
}

#endif /* BSD4_4 */
#endif /* ultrix */
#endif /* __svr4__ */


static int getfsquota(vol, uid, dq)
    struct vol          *vol;
    const int           uid;
    struct dqblk        *dq;
{
#ifdef __svr4__
    struct quotctl	qc;

    qc.op = Q_GETQUOTA;
    qc.uid = uid;
    qc.addr = (caddr_t)dq;
    if ( ioctl( vol->v_qfd, Q_QUOTACTL, &qc ) < 0 ) {
	return( AFPERR_PARAM );
    }

#else /* __svr4__ */
#ifdef ultrix
    if ( quota( Q_GETDLIM, uid, vol->v_gvs, dq ) != 0 ) {
	return( AFPERR_PARAM );
    }
#else ultrix

#ifndef USRQUOTA
#define USRQUOTA   0
#endif

#ifndef QCMD
#define QCMD(a,b)  (a)
#endif

#ifndef TRU64
    /* for group quotas. we only use these if the user belongs
     * to one group. */
    struct dqblk        dqg;

    memset(&dqg, 0, sizeof(dqg));
#endif /* TRU64 */

#ifdef BSD4_4
    if ( quotactl( vol->v_gvs, QCMD(Q_GETQUOTA,USRQUOTA), 
		   uid, (char *)dq ) != 0 ) {
	return( AFPERR_PARAM );
    }

    if (ngroups == 1) 
      quotactl(vol->v_gvs, QCMD(Q_GETQUOTA, GRPQUOTA),
		   groups[0], (char *) &dqg);
      
#elif defined(TRU64)
	if ( seteuid( getuid() ) == 0 ) {
		if ( quotactl( vol->v_path, QCMD(Q_GETQUOTA, USRQUOTA),
				uid, (char *)dq ) != 0 ) {
			seteuid( uid );
			return ( AFPERR_PARAM );
		}
		seteuid( uid );
	}

#else /* BSD4_4 */
    if ( quotactl(QCMD(Q_GETQUOTA, USRQUOTA), vol->v_gvs, uid,
		  (caddr_t) dq ) != 0 ) {
	return( AFPERR_PARAM );
    }

    if (ngroups == 1) 
      quotactl(QCMD(Q_GETQUOTA, GRPQUOTA), vol->v_gvs, 
	       groups[0], (char *) &dqg);
#endif  /* BSD4_4 */

#ifndef TRU64
    /* set stuff up for group quotas if necessary */

    /* max(user blocks, group blocks) */
    if (dqg.dqb_curblocks && (dq->dqb_curblocks < dqg.dqb_curblocks))
      dq->dqb_curblocks = dqg.dqb_curblocks;
    
    /* min(user limit, group limit) */
    if (dqg.dqb_bhardlimit && (!dq->dqb_bhardlimit ||
	 (dq->dqb_bhardlimit > dqg.dqb_bhardlimit)))
      dq->dqb_bhardlimit = dqg.dqb_bhardlimit;

    /* ditto */
    if (dqg.dqb_bsoftlimit && (!dq->dqb_bsoftlimit ||
	 (dq->dqb_bsoftlimit > dqg.dqb_bsoftlimit)))
      dq->dqb_bsoftlimit = dqg.dqb_bsoftlimit;

    /* ditto */
    if (dqg.dqb_btimelimit && (!dq->dqb_btimelimit ||
	 (dq->dqb_btimelimit > dqg.dqb_btimelimit)))
      dq->dqb_btimelimit = dqg.dqb_btimelimit;

#endif /* TRU64 */

#endif /* ultrix */
#endif /* __svr4__ */

    return AFP_OK;
}


static int getquota( vol, dq, bsize)
    struct vol		*vol;
    struct dqblk	*dq;
    const u_int32_t     bsize;
{
    char *p;

#ifdef __svr4__
    char		buf[ MAXPATHLEN + 1];

    if ( vol->v_qfd == -1 && vol->v_gvs == NULL) {
	if (( p = mountp( vol->v_path, &vol->v_nfs)) == NULL ) {
	    syslog( LOG_INFO, "getquota: mountp %s fails", vol->v_path );
	    return( AFPERR_PARAM );
	}

	if (vol->v_nfs) {
	  if (( vol->v_gvs = (char *)malloc( strlen( p ) + 1 )) == NULL ) {
	    syslog( LOG_ERR, "getquota: malloc: %m" );
	    return AFPERR_MISC;
	  }
	  strcpy( vol->v_gvs, p );

	} else {
	  sprintf( buf, "%s/quotas", p );
	  if (( vol->v_qfd = open( buf, O_RDONLY, 0 )) < 0 ) {
	    syslog( LOG_INFO, "open %s: %m", buf );
	    return( AFPERR_PARAM );
	  }
	}
	  
    }
#else
    if ( vol->v_gvs == NULL ) {
	if (( p = special( vol->v_path, &vol->v_nfs )) == NULL ) {
	    syslog( LOG_INFO, "getquota: special %s fails", vol->v_path );
	    return( AFPERR_PARAM );
	}

	if (( vol->v_gvs = (char *)malloc( strlen( p ) + 1 )) == NULL ) {
	    syslog( LOG_ERR, "getquota: malloc: %m" );
	    return AFPERR_MISC;
	}
	strcpy( vol->v_gvs, p );
    }
#endif

    return vol->v_nfs ? getnfsquota(vol, uuid, bsize, dq) : 
      getfsquota(vol, uuid, dq);
}

static int overquota( dqblk )
    struct dqblk	*dqblk;
{
    struct timeval	tv;

    if ( dqblk->dqb_curblocks < dqblk->dqb_bsoftlimit ) {
	return( 0 );
    }
#ifdef ultrix
    if ( dqblk->dqb_bwarn ) {
	return( 0 );
    }
#else /* ultrix */
    if ( gettimeofday( &tv, 0 ) < 0 ) {
	syslog( LOG_ERR, "overquota: gettimeofday: %m" );
	return( AFPERR_PARAM );
    }
    if ( !dqblk->dqb_btimelimit || dqblk->dqb_btimelimit > tv.tv_sec ) {
	return( 0 );
    }
#endif /* ultrix */
    return( 1 );
}

/*
 * This next bit is basically for linux -- everything is fine
 * if you use 1k blocks... but if you try (for example) to mount
 * a volume via nfs from a netapp (which might use 4k blocks) everything
 * gets reported improperly.  I have no idea about dbtob on other
 * platforms.
 */

#ifdef HAVE_BROKEN_DBTOB
#undef dbtob
#define dbtob(a, b)	((VolSpace)((VolSpace)(a) * (VolSpace)(b)))
#define HAVE_2ARG_DBTOB
#endif

#ifndef dbtob
#define dbtob(a)       ((a) << 10)
#endif

/* i do the cast to VolSpace here to make sure that 64-bit shifts 
   work */
#ifdef HAVE_2ARG_DBTOB
#define tobytes(a, b)  dbtob((VolSpace) (a), (VolSpace) (b))
#else 
#define tobytes(a, b)  dbtob((VolSpace) (a))
#endif
int uquota_getvolspace( vol, bfree, btotal, bsize)
    const struct vol	*vol;
    VolSpace	*bfree, *btotal;
    const u_int32_t bsize;
{
    struct dqblk	dqblk;

    if (getquota( vol, &dqblk, bsize) != 0 ) {
      return( AFPERR_PARAM );
    }

    /* no limit set for this user. it might be set in the future. */
    if (dqblk.dqb_bsoftlimit == 0 && dqblk.dqb_bhardlimit == 0) {
       *btotal = *bfree = ~((VolSpace) 0);
    } else if ( overquota( &dqblk )) {
	*btotal = tobytes( dqblk.dqb_bhardlimit, bsize );
	*bfree = tobytes( dqblk.dqb_bhardlimit, bsize ) - 
	  tobytes( dqblk.dqb_curblocks, bsize );
    } else {
	*btotal = tobytes( dqblk.dqb_bsoftlimit, bsize );
	*bfree = tobytes( dqblk.dqb_bsoftlimit, bsize  ) - 
	  tobytes( dqblk.dqb_curblocks, bsize );
    }

    return( AFP_OK );
}
#endif
