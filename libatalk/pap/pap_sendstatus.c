#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

int pap_sendstatus(PAP pap)
{
  struct atp_block atpb; 
  u_int8_t buf[PAP_HDRSIZ];
  
  buf[ 0 ] = 0;
  buf[ 1 ] = PAP_SENDSTATUS;
  buf[ 2 ] = buf[ 3 ] = 0;
  atpb.atp_saddr =&pap->pap_sat;
  atpb.atp_sreqdata = buf;
  atpb.atp_sreqdlen = 4;	/* bytes in SendStatus request */
  atpb.atp_sreqto = 2;		/* retry timer */
  atpb.atp_sreqtries = 5;		/* retry count */
  if ( atp_sreq( satp, &atpb, 1, 0 ) < 0 ) {
    return -1;
  }

  return 0;
}
