/* moved over from bin/pap/pap.c */

static struct {
  PAP pap;
  int tickle;
  pid_t pid;
} client;

static void tickle_handler()
{
  if (client.tickle++ < 4)
    pap_tickle(client.pap, client.pap->pap_connid, &client.pap->pap_sat);
  else {
    kill(client.pid, SIGTERM);
    syslog(LOG_ERR, "pap_alarm: connection timed out.");
    exit(1);
  }
}


PAP pap_open(PAP pap, struct nbpnve *nn, u_int8_t quantum, int flags)
{
    struct atp_block atpb;
    struct timeval stv, tv;
    u_int16_t waiting;
    pid_t pid;

    if (!pap->inited) {
      pap->pap_connid = getpid() & 0xff;
      pap->cmdbuf[ 0 ] = pap->pap_connid;
      pap->cmdbuf[ 1 ] = PAP_OPEN;
      pap->cmdbuf[ 2 ] = pap->cmdbuf[ 3 ] = 0;
      pap->cmdbuf[ 4 ] = atp_sockaddr( pap->pap_atp )->sat_port;
      pap->cmdbuf[ 5 ] = quantum;	/* flow quantum */
      
      if ( gettimeofday( &stv, NULL ) < 0 ) {
	perror( "gettimeofday" );
	return NULL;
      }
      
      for (;;) {
	if ( flags & PAPFLAG_CUTS ) {
	  waiting = 0xffff;
	} else {
	  if ( gettimeofday( &tv, NULL ) < 0 ) {
	    perror( "gettimeofday" );
	    return NULL;
	  }
	  waiting = htons( tv.tv_sec - stv.tv_sec );
	}
	
	memcpy(pap->cmdbuf + 6, &waiting, sizeof( waiting ));
	atpb.atp_saddr = &nn->nn_sat;
	atpb.atp_sreqdata = pap->cmdbuf;
	atpb.atp_sreqdlen = 8;		/* bytes in OpenConn request */
	atpb.atp_sreqto = 2;		/* retry timer */
	atpb.atp_sreqtries = 5;		/* retry count */
	if ( atp_sreq( atp, &atpb, 1, ATP_XO ) < 0 ) {
	  perror( "atp_sreq" );
	  return NULL;
	}
	
	iov.iov_base = pap->data;
	iov.iov_len = sizeof( rbuf );
	atpb.atp_rresiov = &iov;
	atpb.atp_rresiovcnt = 1;
	if ( atp_rresp( atp, &atpb ) < 0 ) {
	  perror( "atp_rresp" );
	  if ( connattempts-- <= 0 ) {
	    fprintf( stderr, "Can't connect!\n" );
	    return NULL;
	  }
	  continue;
	}
	
	/* sanity */
	if ( iov.iov_len < 8 || pap->data[ 0 ] != pap->pap_connid ||
	     pap->data[ 1 ] != PAP_OPENREPLY ) {
	  fprintf( stderr, "Bad response!\n" );
	  continue;	/* This is weird, since TIDs must match... */
	}
	
	if ( isatty( 1 )) {
	  printf( "%.*s\n", iov.iov_len - 9, iov.iov_base + 9 );
	}
	updatestatus( iov.iov_base + 9, iov.iov_len - 9 );
	
	bcopy( &rbuf[ 6 ], &result, sizeof( result ));
	if ( result != 0 ) {
	  sleep( 2 );
	} else {
	  memcpy(&pap->pap_sat, &nn.nn_sat, sizeof( struct sockaddr_at ));
	  pap->pap_sat.sat_port = rbuf[ 4 ];
	  pap->pap_quantum = rbuf[ 5 ];
	  break;
	}
      }
      
      if ( isatty( 1 )) {
	printf( "Connected to %.*s:%.*s@%.*s.\n",
		nn.nn_objlen, nn.nn_obj,
		nn.nn_typelen, nn.nn_type,
		nn.nn_zonelen, nn.nn_zone );
      }

    /* open a second atp connection */
    if (( atp = atp_open( 0 )) == NULL ) {
	perror( "atp_open" );
	return NULL;
    }

    client.pap = pap;
    client.pid = pid;
    pap->inited = 1;
    }

    /* wait around for tickles */
    
}
