/*
 * $Id: messages.c,v 1.12 2002-01-03 17:29:10 sibaz Exp $
 *
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <string.h>
#include <atalk/afp.h>
#include <syslog.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
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
    int i, rc;
    static int c;
    uid_t euid;

    i=0;
    /* Construct file name SERVERTEXT/message.[pid] */
    filename=malloc(sizeof(SERVERTEXT)+15);
    sprintf(filename, "%s/message.%d", SERVERTEXT, getpid());

#ifdef DEBUG
    syslog(LOG_DEBUG, "Reading file %s ", filename);
#endif /* DEBUG */

    message=fopen(filename, "r");
    if (message==NULL) {
        syslog(LOG_INFO, "Unable to open file %s", filename);
        sprintf(filename, "%s/message", SERVERTEXT);
        message=fopen(filename, "r");
    }

    /* if either message.pid or message exists */
    if (message!=NULL) {
        /* added while loop to get characters and put in servermesg */
        while ((( c=fgetc(message)) != EOF) && (i < (MAXMESGSIZE - 1))) {
            if ( c == '\n')  c = ' ';
            servermesg[i++] = c;
        }
        servermesg[i] = 0;

        /* cleanup */
        fclose(message);

        /* Save effective uid and switch to root to delete file. */
        /* Delete will probably fail otherwise, but let's try anyways */
        euid = geteuid();
        if (seteuid(0) < 0) {
            syslog(LOG_ERR, "Could not switch back to root: %m");
        }

        rc = unlink(filename);

        /* Drop privs again, failing this is very bad */
        if (seteuid(euid) < 0) {
            syslog(LOG_ERR, "Could not switch back to uid %d: %m", euid);
        }

        if (rc < 0) {
            syslog(LOG_ERR, "Error deleting %s: %m", filename);
        }
#ifdef DEBUG
        else {
            syslog(LOG_INFO, "Deleted %s", filename);
        }

        syslog(LOG_INFO, "Set server message to \"%s\"", servermesg);
#endif /* DEBUG */
    }
    free(filename);
#endif /* SERVERTEXT */
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
