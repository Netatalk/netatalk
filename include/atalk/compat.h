/*
 * Copyright (c) 1996 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"

#include <signal.h>

/* OpenBSD */
#if defined(__OpenBSD__) && !defined(ENOTSUP)
#define ENOTSUP EOPNOTSUPP
#endif

#if !defined(HAVE_PSELECT) || defined(__OpenBSD__)
extern int pselect(int, fd_set * restrict, fd_set * restrict,
                   fd_set * restrict, const struct timespec * restrict,
                   const sigset_t *restrict);
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

#endif
