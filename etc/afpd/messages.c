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
#include "globals.h"
#include "misc.h"

#define MAXMESGSIZE 199

/* this is only used by afpd children, so it's okay. */
static char servermesg[MAXMESGSIZE] = "";

void setmessage(const char *message)
{
  strncpy(servermesg, message, MAXMESGSIZE);
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
