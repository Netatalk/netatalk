#include <atalk/compat.h>

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
