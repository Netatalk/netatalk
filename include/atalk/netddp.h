/*
 * Copyright (c) 1999 Adrian Sun (asun@zoology.washington.edu)
 * All Rights Reserved. See COPYRIGHT.
 *
 * this provides a generic interface to the ddp layer. with this, we
 * should be able to interact with any appletalk stack that allows
 * direct access to the ddp layer. right now, only os x server's ddp
 * layer and the generic socket based interfaces are understood.
 */

#ifndef _ATALK_NETDDP_H
#define _ATALK_NETDDP_H 1

#ifndef NO_DDP

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <netatalk/at.h>

extern int netddp_open(struct sockaddr_at *, struct sockaddr_at *);

static inline int netddp_close(int filedes)
{
    return close(filedes);
}

static inline ssize_t netddp_sendto(int s, const void *msg, size_t len,
                                    int flags, const struct sockaddr *to, socklen_t tolen)
{
    return sendto(s, msg, len, flags, to, tolen);
}

static inline ssize_t netddp_recvfrom(int s, void *buf, size_t len,
                                      int flags, struct sockaddr *from,
                                      socklen_t *fromlen)
{
    return recvfrom(s, buf, len, flags, from, fromlen);
}

#endif  /* NO_DDP */
#endif /* netddp.h */

