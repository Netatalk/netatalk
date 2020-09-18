/* 
   Unix SMB/CIFS implementation.
   Samba database functions
   Copyright (C) Anton Blanchard                   2001
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#define STANDALONE 1

#if HAVE_CONFIG_H
#include <config.h>
#endif

#if STANDALONE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <signal.h>
#include "spinlock.h"

#define DEBUG
#else
#include "includes.h"
#endif

int tdb_create_rwlocks(int fd, unsigned int hash_size) { return 0; }
int tdb_spinlock(TDB_CONTEXT *tdb, int list, int rw_type) { return -1; }
int tdb_spinunlock(TDB_CONTEXT *tdb, int list, int rw_type) { return -1; }

/* Non-spinlock version: remove spinlock pointer */
int tdb_clear_spinlocks(TDB_CONTEXT *tdb)
{
	tdb_off off = (tdb_off)((char *)&tdb->header.rwlocks
				- (char *)&tdb->header);

	tdb->header.rwlocks = 0;
	if (lseek(tdb->fd, off, SEEK_SET) != off
	    || write(tdb->fd, (void *)&tdb->header.rwlocks,
		     sizeof(tdb->header.rwlocks)) 
	    != sizeof(tdb->header.rwlocks))
		return -1;
	return 0;
}
