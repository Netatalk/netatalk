#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <syslog.h>
#include <sys/socket.h>

#include <atalk/atp.h>
#include <atalk/asp.h>

/* send off a tickle */
void asp_tickle(ASP asp, const u_int8_t sid, struct sockaddr_at *sat)
{
  struct atp_block atpb;
  char buf[ASP_HDRSIZ];

  buf[ 0 ] = ASPFUNC_TICKLE;
  buf[ 1 ] = sid;
  buf[ 2 ] = buf[ 3 ] = 0;

  atpb.atp_saddr = sat;
  atpb.atp_sreqdata = buf;
  atpb.atp_sreqdlen = sizeof(buf);
  atpb.atp_sreqto = 0;
  atpb.atp_sreqtries = 1;
  if ( atp_sreq( asp->asp_atp, &atpb, 0, 0 ) < 0 ) {
    syslog( LOG_ERR, "atp_sreq: %m" );
  }
}
