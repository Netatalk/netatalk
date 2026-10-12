/*!
 * @file
 * @brief strnlen, vasprintf, and asprintf for systems without them
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdarg.h>
#include <stdio.h>

#include <atalk/compat.h>

#if !defined HAVE_DIRFD && defined SOLARIS
#include <dirent.h>
int dirfd(DIR *dir)
{
    return dir->d_fd;
}
#endif

#ifndef HAVE_STRNLEN
/*!
 * @brief Return the length of a string, capped at max
 *
 * @param[in] s    string
 * @param[in] max  byte limit
 * @returns the number of bytes before the NUL, or max without a NUL in the
 *          first max bytes
 */
size_t strnlen(const char *s, size_t max)
{
    size_t len;

    for (len = 0; len < max; len++) {
        if (s[len] == '\0') {
            break;
        }
    }

    return len;
}
#endif

#ifndef HAVE_VASPRINTF
/*!
 * @brief Format into a newly allocated string, for systems without vasprintf
 *
 * The buffer starts at 64 bytes and grows until vsnprintf fits.
 *
 * @param[out] ret  receives the string, or NULL after a failed realloc
 * @param[in]  fmt  printf format
 * @param[in]  ap   arguments
 * @returns the string length, -1 after a failed realloc, or 0 without a string
 *          when the first malloc fails
 */
int vasprintf(char **ret, const char *fmt, va_list ap)
{
    int n, size = 64;
    char *p, *np;

    if ((p = malloc(size)) == NULL) {
        return NULL;
    }

    while (1) {
        /* Try to print in the allocated space. */
        n = vsnprintf(p, size, fmt, ap);

        /* If that worked, return the string. */
        if (n > -1 && n < size) {
            *ret = p;
            return n;
        }

        /* Else try again with more space. */
        if (n > -1) {
            /* glibc 2.1 */
            /* precisely what is needed */
            size = n + 1;
        } else {
            /* glibc 2.0 */
            /* twice the old size */
            size *= 2;
        }

        if ((np = realloc(p, size)) == NULL) {
            free(p);
            *ret = NULL;
            return -1;
        } else {
            p = np;
        }
    }
}
#endif

#ifndef HAVE_ASPRINTF
/*!
 * @brief Format into a newly allocated string, for systems without asprintf
 *
 * @param[out] strp  receives the string
 * @param[in]  fmt   printf format
 * @param[in]  ...   arguments
 * @returns the result of vasprintf
 */
int asprintf(char **strp, const char *fmt, ...)
{
    va_list ap;
    int len;
    va_start(ap, fmt);
    len = vasprintf(strp, fmt, ap);
    va_end(ap);
    return len;
}
#endif
