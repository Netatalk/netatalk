/*
 * Copyright (c) 1999 Adrian Sun (asun@zoology.washington.edu)
 * All Rights Reserved. See COPYRIGHT.
 */

/*!
 * @file
 * @brief Portable DDP socket lifecycle and datagram I/O interface
 *
 * This provides a generic interface to the DDP layer. With this, we
 * should be able to interact with any AppleTalk stack that allows
 * direct access to the DDP layer.
 */

#ifndef _ATALK_NETDDP_H
#define _ATALK_NETDDP_H 1

#ifndef NO_DDP

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <netatalk/at.h>

extern int netddp_open(struct sockaddr_at *, struct sockaddr_at *);

/*! @brief Close a DDP socket */
static inline int netddp_close(int filedes)
{
    return close(filedes);
}

/*! @brief Send a DDP datagram through sendto */
static inline ssize_t netddp_sendto(int s, const void *msg, size_t len,
                                    int flags, const struct sockaddr *to, socklen_t tolen)
{
    return sendto(s, msg, len, flags, to, tolen);
}

/*! @brief Receive a DDP datagram through recvfrom */
static inline ssize_t netddp_recvfrom(int s, void *buf, size_t len,
                                      int flags, struct sockaddr *from,
                                      socklen_t *fromlen)
{
    return recvfrom(s, buf, len, flags, from, fromlen);
}

#endif  /* NO_DDP */
#endif /* netddp.h */

