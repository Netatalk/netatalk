/* send a tickle */
void pap_tickle(PAP pap, const u_int8_t connid, struct sockaddr_at *sat)
{
  struct atp_block atpb; 
  u_int8_t buf[PAP_HDRSIZ];

  buf[ 0 ] = connid;
  buf[ 1 ] = PAP_TICKLE;
  buf[ 2 ] = buf[ 3 ] = 0;

  atpb.atp_saddr = sat;
  atpb.atp_sreqdata = buf;
  atpb.atp_sreqdlen = sizeof(buf);	/* bytes in Tickle request */
  atpb.atp_sreqto = 0;		/* retry timer */
  atpb.atp_sreqtries = 1;	/* retry count */
  if ( atp_sreq( pap->pap_atp, &atpb, 0, 0 ) < 0 ) {
    syslog(LOG_ERR, "atp_sreq: %m");
  }
}
