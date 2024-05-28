/*
 * Copyright (c) 1980, 1990, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Robert Elz at The University of Melbourne.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Ported for AIX (jfs) by Joerg Schumacher (J.Schumacher@tu-bs.de) at the
 * Technische Universitaet Braunschweig, FRG
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#if !defined(NO_QUOTA_SUPPORT) || defined(HAVE_LIBQUOTA)
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h> /* for DEV_BSIZE */
#include <sys/time.h>
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif /* HAVE_NETDB_H */
#include <netinet/in.h>
#ifndef PORTMAP
#define PORTMAP 1
#endif
#include <rpc/rpc.h>
#include <rpc/pmap_prot.h>
#include <rpcsvc/rquota.h>


#include <atalk/afp.h>
#include <atalk/logger.h>

#include "unix.h"

/* lifted (with modifications) from the bsd quota program */
static int
callaurpc(struct vol *vol,
    u_long prognum, u_long versnum, u_long procnum,
    xdrproc_t inproc, char *in,
    xdrproc_t outproc, char *out)
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

int getnfsquota(struct vol *vol, const int uid, const u_int32_t bsize,
                struct dqblk *dqp)
{

    struct getquota_args gq_args;
    struct getquota_rslt gq_rslt;
    struct timeval tv;
    char *hostpath;

    /* figure out the host and path */
    if ((hostpath = strchr(vol->v_gvs, ':')) == NULL) {
        LOG(log_error, logtype_afpd, "can't find hostname for %s", vol->v_gvs);
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
        LOG(log_info, logtype_afpd, "nfsquota: can't retrieve nfs quota information. \
            make sure that rpc.rquotad is running on %s.", vol->v_gvs);
        *hostpath = ':';
        return AFPERR_PARAM;
    }

    switch (gq_rslt.GQR_STATUS) {
    case Q_NOQUOTA:
        break;

    case Q_EPERM:
        LOG(log_error, logtype_afpd, "nfsquota: quota permission error, host: %s",
            vol->v_gvs);
        break;

    case Q_OK: /* we only copy the bits that we need. */
        gettimeofday(&tv, NULL);

#if defined(__svr4__)
        /* why doesn't using bsize work? */
#define NFS_BSIZE gq_rslt.GQR_RQUOTA.rq_bsize / DEV_BSIZE
#else /* __svr4__ */
        /* NOTE: linux' rquotad program doesn't currently report the
        * correct rq_bsize. */
	/* NOTE: This is integer division and can introduce rounding errors */
#define NFS_BSIZE gq_rslt.GQR_RQUOTA.rq_bsize / bsize
#endif /* __svr4__ */

        dqp->dqb_bhardlimit =
            gq_rslt.GQR_RQUOTA.rq_bhardlimit*NFS_BSIZE;
        dqp->dqb_bsoftlimit =
            gq_rslt.GQR_RQUOTA.rq_bsoftlimit*NFS_BSIZE;
        dqp->dqb_curblocks =
            gq_rslt.GQR_RQUOTA.rq_curblocks*NFS_BSIZE;

        dqp->dqb_btimelimit =
            tv.tv_sec + gq_rslt.GQR_RQUOTA.rq_btimeleft;

        *hostpath = ':';
        return AFP_OK;
        break;

    default:
        LOG(log_info, logtype_afpd, "bad rpc result, host: %s", vol->v_gvs);
        break;
    }

    *hostpath = ':';
    return AFPERR_PARAM;
}
#endif /* ! NO_QUOTA_SUPPORT || HAVE_LIBQUOTA */
