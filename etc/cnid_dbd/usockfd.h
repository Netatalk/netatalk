/*
 * Copyright (C) Joerg Lenneis 2003
 * All Rights Reserved.  See COPYING.
 */

#ifndef CNID_DBD_USOCKFD_H
#define CNID_DBD_USOCKFD_H 1



#include <atalk/cnid_dbd_private.h>


int      usockfd_create  (char *, mode_t, int);
int      tsockfd_create  (char *, char *, int);
int      usockfd_check   (int, const sigset_t *);

#ifndef OSSH_ALIGNBYTES
#define OSSH_ALIGNBYTES (sizeof(int) - 1)
#endif
#ifndef __CMSG_ALIGN
#ifndef uint
#define uint unsigned int
#endif
#define __CMSG_ALIGN(p) (((uint)(p) + OSSH_ALIGNBYTES) &~ OSSH_ALIGNBYTES)
#endif

/* Length of the contents of a control message of length len */
#ifndef CMSG_LEN
#define CMSG_LEN(len)   (__CMSG_ALIGN(sizeof(struct cmsghdr)) + (len))
#endif

/* Length of the space taken up by a padded control message of length len */
#ifndef CMSG_SPACE
#define CMSG_SPACE(len) (__CMSG_ALIGN(sizeof(struct cmsghdr)) + __CMSG_ALIGN(len))
#endif



#endif /* CNID_DBD_USOCKFD_H */
