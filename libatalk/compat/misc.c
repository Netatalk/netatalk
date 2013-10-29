#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdarg.h>

#include <atalk/compat.h>

#if !defined HAVE_DIRFD && defined SOLARIS
#include <dirent.h>
int dirfd(DIR *dir)
{
    return dir->d_fd;
}
#endif

#ifndef HAVE_STRNLEN
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
int vasprintf(char **ret, const char *fmt, va_list ap)
{
    int n, size = 64;
    char *p, *np;

    if ((p = malloc(size)) == NULL)
        return NULL;

    while (1) {
        /* Try to print in the allocated space. */
        n = vsnprintf(p, size, fmt, ap);
        /* If that worked, return the string. */
        if (n > -1 && n < size) {
            *ret = p;
            return n;
        }
        /* Else try again with more space. */
        if (n > -1)    /* glibc 2.1 */
            size = n+1; /* precisely what is needed */
        else           /* glibc 2.0 */
            size *= 2;  /* twice the old size */
        if ((np = realloc (p, size)) == NULL) {
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
