/*
   Copyright (c) 2010 Frank Lahm <franklahm@gmail.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 */

#ifndef ERRCHECK_H
#define ERRCHECK_H

#define EC_INIT int ret = 0
#define EC_CLEANUP cleanup
#define EC_EXIT return ret

/* 
 * We have these macros:
 * EC_ZERO_LOG
 * EC_ZERO
 * EC_NEG1_LOG
 * EC_NEG1
 * EC_NULL_LOG
 * EC_NULL
 */

/* check for return val 0 which is ok, every other is an error, prints errno */
#define EC_ZERO_LOG(a) \
    do { \
        if ((a) != 0) { \
            LOG(log_error, logtype_default, "%s failed: %s" #a, strerror(errno)); \
            ret = -1; \
            goto cleanup; \
        } \
    } while (0)

/* check for return val 0 which is ok, every other is an error */
#define EC_ZERO(a) \
    do { \
        if ((a) != 0) { \
            ret = -1; \
            goto cleanup; \
        } \
    } while (0)

/* check for return val 0 which is ok, every other is an error, prints errno */
#define EC_NEG1_LOG(a) \
    do { \
        if ((a) == -1) { \
            LOG(log_error, logtype_default, "%s failed: %s" #a, strerror(errno)); \
            ret = -1; \
            goto cleanup; \
        } \
    } while (0)

/* check for return val 0 which is ok, every other is an error */
#define EC_NEG1(a) \
    do { \
        if ((a) == -1) { \
            ret = -1; \
            goto cleanup; \
        } \
    } while (0)

/* check for return val != NULL, prints errno */
#define EC_NULL_LOG(a) \
    do { \
        if ((a) == NULL) { \
            LOG(log_error, logtype_default, "%s failed: %s" #a, strerror(errno)); \
            ret = -1; \
            goto cleanup; \
        } \
    } while (0)

/* check for return val != NULL */
#define EC_NULL(a) \
    do { \
        if ((a) == NULL) { \
            ret = -1; \
            goto cleanup; \
        } \
    } while (0)

#endif /* ERRCHECK_H */
