#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#if !defined HAVE_DIRFD && defined SOLARIS
#include <dirent.h>
int dirfd(DIR *dir)
{
    return dir->dd_fd;
}
#endif
