/*
 * $Id: pap_close.c,v 1.3 2001-06-29 14:14:47 rufustfirefly Exp $
 *
 * close the connection
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

int pap_close(PAP pap)
{
  struct atp_block atpb;
  struct iovec iov;
  unsigned char buf[PAP_HDRSIZ];
  int err = -1;

  buf[ 0 ] = pap->pap_connid;
  buf[ 1 ] = PAP_CLOSE;
  buf[ 2 ] = buf[ 3 ] = 0;

  atpb.atp_saddr = &pap->pap_sat;
  atpb.atp_sreqdata = buf;
  atpb.atp_sreqdlen = sizeof(buf);	/* bytes in CloseConn request */
  atpb.atp_sreqto = 2;		        /* retry timer */
  atpb.atp_sreqtries = 5;		/* retry count */
  if (atp_sreq( atp, &atpb, 1, ATP_XO ) < 0) {
    goto close_done;
  }

  /* check for CloseConnReply */
  iov.iov_base = pap->pap_data;
  iov.iov_len = sizeof( pap->pap_data );
  atpb.atp_rresiov = &iov;
  atpb.atp_rresiovcnt = 1;
  if ( atp_rresp( pap->pap_atp, &atpb ) < 0 ) {
    goto close_done;
  }
  
  /* sanity */
  if ( iov.iov_len != 4 || pap->pap_data[ 0 ] != pap->pap_connid ||
       pap->pap_data[ 1 ] != PAP_CLOSEREPLY ) {
    syslog(LOG_ERR, "pap_close: Bad response!");
    goto close_done;
  }
  err = 0;

close_done:
  atp_close(pap->pap_atp);
  free(pap);
  return err;
}
