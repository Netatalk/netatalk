/* taken from bin/pap/pap.c */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

int pap_read(PAP pap)
{
    struct atp_block atpb;
    u_int16_t seq;

    pap->cmdbuf[0] = pap->pap_connid;
    pap->cmdbuf[1] = PAP_READ;
    if ( ++pap->pap_seq == 0 ) 
      pap->pap_seq = 1;

    seq = htons( pap->pap_seq );
    memcpy(pap->cmdbuf + 2, &seq, sizeof(seq));
    atpb.atp_saddr = &pap->pap_sat;
    atpb.atp_sreqdata = pap->cmdbuf;
    atpb.atp_sreqdlen = sizeof(pap->cmdbuf);	/* bytes in SendData request */
    atpb.atp_sreqto = 15;		/* retry timer */
    atpb.atp_sreqtries = ATP_TRIES_INFINITE;	/* retry count */
    if ( atp_sreq( pap->pap_atp, &atpb, pap->oquantum, ATP_XO ) < 0 ) {
      return -1;
    }

    return 0;
}
