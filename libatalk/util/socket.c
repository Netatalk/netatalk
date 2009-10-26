/*
   $Id: socket.c,v 1.1 2009-10-26 12:35:56 franklahm Exp $
   Copyright (c) 2009 Frank Lahm <franklahm@gmail.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <unistd.h>
#include <fcntl.h>

int setnonblock(int fd, int cmd)
{
    int ofdflags;
    int fdflags;

    if ((fdflags = ofdflags = fcntl(fd, F_GETFL, 0)) == -1)
        return -1;

    if (cmd)
        fdflags |= O_NONBLOCK;
    else
        fdflags &= ~O_NONBLOCK;

    if (fdflags != ofdflags)
        if (fcntl(fd, F_SETFL, fdflags) == -1)
            return -1;

    return 0;
}
