/*
 * Copyright (c) 1996 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifndef _ATALK_COMPAT_H
#define _ATALK_COMPAT_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <signal.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>

/* OpenBSD */
#if defined(__OpenBSD__) && !defined(ENOTSUP)
#define ENOTSUP EOPNOTSUPP
#endif

#if !defined(HAVE_PSELECT) || defined(__OpenBSD__)
extern int pselect(int, fd_set *, fd_set *, fd_set *, const struct timespec *,
                   const sigset_t *);
#endif

#ifndef HAVE_STRNLEN
extern size_t strnlen(const char *s, size_t n);
#endif

#ifndef HAVE_STRLCPY
extern size_t strlcpy(char *, const char *, size_t);
#endif

#ifndef HAVE_STRLCAT
extern size_t strlcat(char *, const char *, size_t);
#endif

#ifndef HAVE_VASPRINTF
#include <stdio.h>
#include <stdarg.h>
extern int vasprintf(char **ret, const char *fmt, va_list ap);
#endif

/* Secure memory clearing - prefer memset_explicit (C23) over explicit_bzero */
#if !defined(HAVE_MEMSET_EXPLICIT) && !defined(HAVE_EXPLICIT_BZERO)
#include <stddef.h>
extern void explicit_bzero(void *s, size_t n);
#elif defined(HAVE_MEMSET_EXPLICIT) && !defined(HAVE_EXPLICIT_BZERO)
#include <string.h>
#define explicit_bzero(s, n) memset_explicit((s), 0, (n))
#endif

static inline struct timespec atalk_stat_mtime_timespec(const struct stat *st)
{
    struct timespec ts;
#if defined(HAVE_STRUCT_STAT_ST_MTIM)
    ts = st->st_mtim;
#elif defined(HAVE_STRUCT_STAT_ST_MTIMESPEC)
    ts = st->st_mtimespec;
#else
    ts.tv_sec = st->st_mtime;
    ts.tv_nsec = 0;
#endif
    return ts;
}

static inline struct timespec atalk_stat_atime_timespec(const struct stat *st)
{
    struct timespec ts;
#if defined(HAVE_STRUCT_STAT_ST_ATIM)
    ts = st->st_atim;
#elif defined(HAVE_STRUCT_STAT_ST_ATIMESPEC)
    ts = st->st_atimespec;
#else
    ts.tv_sec = st->st_atime;
    ts.tv_nsec = 0;
#endif
    return ts;
}

static inline void atalk_timespec_to_timeval(struct timeval *tv,
        const struct timespec *ts)
{
    tv->tv_sec = ts->tv_sec;
    tv->tv_usec = ts->tv_nsec / 1000;
}

#endif /* _ATALK_COMPAT_H */
