/*
 * $Id: nfsquota.c,v 1.3 2001-06-20 18:33:04 rufustfirefly Exp $
 *
 * parts of this are lifted from the bsd quota program and are
 * therefore under the following copyright:
 *
 * Copyright (c) 1980, 1990, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Robert Elz at The University of Melbourne.
 *
 * Ported for AIX (jfs) by Joerg Schumacher (J.Schumacher@tu-bs.de) at the
 * Technische Universitaet Braunschweig, FRG
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h> /* for DEV_BSIZE */
#include <sys/time.h>  /* <rpc/rpc.h> on ultrix doesn't include this */
#include <syslog.h>

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif /* HAVE_NETDB_H */
#include <netinet/in.h>
#include <rpc/rpc.h>
#include <rpc/pmap_prot.h>
#include <rpcsvc/rquota.h>

#include <atalk/afp.h>

#include "unix.h"

#ifndef NO_QUOTA_SUPPORT
/* lifted (with modifications) from the bsd quota program */
static int 
callaurpc(vol, prognum, versnum, procnum, inproc, in, outproc, out)
	struct vol *vol;
	u_long prognum, versnum, procnum;
	xdrproc_t inproc, outproc;
	char *in, *out;
{
	enum clnt_stat clnt_stat;
	struct timeval tottimeout;
 
	if (!vol->v_nfsclient) {
	  struct hostent *hp;
	  struct sockaddr_in server_addr;
	  struct timeval timeout;
	  int socket = RPC_ANYSOCK;
	  
	  if ((hp = gethostbyname(vol->v_gvs)) == NULL)
	    return ((int) RPC_UNKNOWNHOST);
	  timeout.tv_usec = 0;
	  timeout.tv_sec = 6;
	  memcpy(&server_addr.sin_addr, hp->h_addr, hp->h_length);
	  server_addr.sin_family = AF_INET;
	  server_addr.sin_port =  0;
	  
	  if ((vol->v_nfsclient = (void *) 
	       clntudp_create(&server_addr, prognum, versnum, 
			      timeout, &socket)) == NULL)
	    return ((int) rpc_createerr.cf_stat);
	  
	  ((CLIENT *) vol->v_nfsclient)->cl_auth = authunix_create_default();
	}

	tottimeout.tv_sec = 10;
	tottimeout.tv_usec = 0;
	clnt_stat = clnt_call((CLIENT *) vol->v_nfsclient, procnum,
			      inproc, in, outproc, out, tottimeout);
	return ((int) clnt_stat);
}


/* sunos 4 machines structure things a little differently. */
#ifdef USE_OLD_RQUOTA
#define GQR_STATUS gqr_status
#define GQR_RQUOTA gqr_rquota
#else /* USE_OLD_RQUOTA */
#define GQR_STATUS status
#define GQR_RQUOTA getquota_rslt_u.gqr_rquota
#endif /* USE_OLD_RQUOTA */

int getnfsquota(const struct vol *vol, const int uid, const u_int32_t bsize,
		struct dqblk *dqp)
{

  struct getquota_args gq_args;
  struct getquota_rslt gq_rslt;
  struct timeval tv;
  char *hostpath;
 
  /* figure out the host and path */
  if ((hostpath = strchr(vol->v_gvs, ':')) == NULL) {
    syslog(LOG_ERR, "can't find hostname for %s", vol->v_gvs);
    return AFPERR_PARAM;
  }
 
  if (*(hostpath + 1) != '/') 
    return AFPERR_PARAM;

  /* separate host from hostpath */
  *hostpath = '\0';
  
  gq_args.gqa_pathp = hostpath + 1;
  gq_args.gqa_uid = uid;

  if(callaurpc(vol, RQUOTAPROG, RQUOTAVERS, RQUOTAPROC_GETQUOTA, 
	       (xdrproc_t) xdr_getquota_args, (char *) &gq_args,
	       (xdrproc_t) xdr_getquota_rslt, (char *) &gq_rslt) != 0) {
    syslog(LOG_INFO, "nfsquota: can't retrieve nfs quota information. \
make sure that rpc.rquotad is running on %s.", vol->v_gvs);
    *hostpath = ':';
    return AFPERR_PARAM;
  }
 
  switch (gq_rslt.GQR_STATUS) {
  case Q_NOQUOTA:
    break;

  case Q_EPERM:
    syslog(LOG_ERR, "nfsquota: quota permission error, host: %s\n",
	   vol->v_gvs);
    break;

  case Q_OK: /* we only copy the bits that we need. */
    gettimeofday(&tv, NULL);

#ifdef __svr4__
    /* why doesn't using bsize work? */
#define NFS_BSIZE (gq_rslt.GQR_RQUOTA.rq_bsize / DEV_BSIZE)
#else /* __svr4__ */
    /* NOTE: linux' rquotad program doesn't currently report the
     * correct rq_bsize. */
#define NFS_BSIZE (gq_rslt.GQR_RQUOTA.rq_bsize / bsize)
#endif /* __svr4__ */

    dqp->dqb_bhardlimit =
      gq_rslt.GQR_RQUOTA.rq_bhardlimit*NFS_BSIZE;
    dqp->dqb_bsoftlimit =
      gq_rslt.GQR_RQUOTA.rq_bsoftlimit*NFS_BSIZE;
    dqp->dqb_curblocks =
      gq_rslt.GQR_RQUOTA.rq_curblocks*NFS_BSIZE;

#ifdef ultrix
    dqp->dqb_bwarn = gq_rslt.GQR_RQUOTA.rq_btimeleft;
#else /* ultrix */
    dqp->dqb_btimelimit =
      tv.tv_sec + gq_rslt.GQR_RQUOTA.rq_btimeleft;
#endif /* ultrix */

    *hostpath = ':';
    return AFP_OK;
    break;

  default:
    syslog(LOG_INFO, "bad rpc result, host: %s\n", vol->v_gvs);
    break;
  }

  *hostpath = ':';
  return AFPERR_PARAM;
}
#endif /* ! NO_QUOTA_SUPPORT */
