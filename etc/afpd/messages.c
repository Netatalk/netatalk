/* 
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <atalk/afp.h>
#include <syslog.h>
#include <unistd.h>
#include "globals.h"
#include "misc.h"

#define MAXMESGSIZE 199

/* this is only used by afpd children, so it's okay. */
static char servermesg[MAXMESGSIZE] = "";

void setmessage(const char *message)
{
  strncpy(servermesg, message, MAXMESGSIZE);
}

void readmessage(void)
{
/* Read server message from file defined as SERVERTEXT */
#ifdef SERVERTEXT
  FILE *message;
  char * filename;
  int i;
  static int c;

  i=0;
  // Construct file name SERVERTEXT/message.[pid]
  filename=malloc(sizeof(SERVERTEXT)+15);
  sprintf(filename, "%s/message.%d", SERVERTEXT, getpid());

  syslog (LOG_DEBUG, "Reading file %s ", filename);
  
  message=fopen(filename, "r");
  if (message==NULL) {
    syslog (LOG_INFO, "Unable to open file %s", filename);
    sprintf(filename, "%s/message", SERVERTEXT);
    message=fopen(filename, "r");
  }

  if (message!=NULL) /* if either message.pid or message exists */
  {
    /* added while loop to get characters and put in servermesg */
    while ((( c=fgetc(message)) != EOF) && (i < (MAXMESGSIZE - 1))) {
      if ( c == '\n')  c = ' ';
      servermesg[i++] = c;
      }
    servermesg[i] = 0;

    /* cleanup */
    fclose(message);
/* Below code can be uncommented if you want to experiment with getting afpd
to delete the text file when it's done.  Currently, it doesn't work.  If you 
can get it to work, delete this comment and enable the code!  */
/*    i=unlink (filename);
    if (i)
      syslog (LOG_INFO, "Error deleting %s: %m", filename);
    else
      syslog (LOG_DEBUG, "Deleted %s", filename);
    free (filename);
 
    syslog (LOG_DEBUG, "Set server message to \"%s\"", servermesg); */
  }
  free(filename);
#endif
}

int afp_getsrvrmesg(obj, ibuf, ibuflen, rbuf, rbuflen)
     AFPObj *obj;
     char *ibuf, *rbuf;
     int ibuflen, *rbuflen;
{
  char *message;
  u_int16_t type, bitmap;

  memcpy(&type, ibuf + 2, sizeof(type));
  memcpy(&bitmap, ibuf + 4, sizeof(bitmap));

  switch (ntohs(type)) {
  case AFPMESG_LOGIN: /* login */
    message = obj->options.loginmesg;
    break;
  case AFPMESG_SERVER: /* server */
    message = servermesg;
    break;
  default:
    *rbuflen = 0;
    return AFPERR_BITMAP;
  }

  if(strlen(message)==0)
    return AFP_OK;

  /* output format:
   * message type:   2 bytes
   * bitmap:         2 bytes
   * message length: 1 byte
   * message:        up to 199 bytes
   */
  memcpy(rbuf, &type, sizeof(type));
  rbuf += sizeof(type);
  memcpy(rbuf, &bitmap, sizeof(bitmap));
  rbuf += sizeof(bitmap);
  *rbuflen = strlen(message);
  if (*rbuflen > MAXMESGSIZE)
    *rbuflen = MAXMESGSIZE;
  *rbuf++ = *rbuflen;
  memcpy(rbuf, message, *rbuflen);

  *rbuflen += 5;

  return AFP_OK;
}
