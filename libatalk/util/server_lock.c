/*
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved. See COPYRIGHT.
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <atalk/compat.h>
#include <atalk/util.h>

/* this creates an open lock file which hangs around until the program
 * dies. it returns the pid. due to problems w/ solaris, this has
 * been changed to do the kill() thing. */
pid_t server_lock(char *program, char *pidfile, int debug)
{
  char buf[10];
  FILE *pf;
  pid_t pid;
  int mask;
  
  mask = umask(022);
  /* check for pid. this can get fooled by stale pid's. */
  if ((pf = fopen(pidfile, "r"))) {
    if (fgets(buf, sizeof(buf), pf) && !kill(pid = atol(buf), 0)) {
      fprintf( stderr, "%s is already running (pid = %d), or the lock file is stale.\n",
	       program, pid);      
      fclose(pf);
      return -1;
    }
    fclose(pf);
  }

  if ((pf = fopen(pidfile, "w")) == NULL) {
    fprintf(stderr, "%s: can't open lock file, \"%s\"\n", program,
	    pidfile);
    return -1;
  }
  umask(mask);

  /*
   * Disassociate from controlling tty.
   */
  if ( !debug ) {
    int		i;

    switch (pid = fork()) {
    case 0 :
      fclose(stdin);
      fclose(stdout);
      fclose(stderr);

      if (( i = open( "/dev/tty", O_RDWR )) >= 0 ) {
	(void)ioctl( i, TIOCNOTTY, 0 );
	setpgid( 0, getpid());
	(void) close(i);
      }
      break;
    case -1 :  /* error */
      perror( "fork" );
    default :  /* server */
      fclose(pf);
      return pid;
    }
  } 

  fprintf(pf, "%d\n", getpid());
  fclose(pf);
  return 0;
}
