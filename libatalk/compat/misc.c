#include "config.h"

#if !defined HAVE_DIRFD && defined SOLARIS
#include <dirent.h>
int dirfd(DIR *dir)
{
    return dir->d_fd;
}
#endif
