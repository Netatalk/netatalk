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

#ifdef USE_SPINLOCKS

/*
 * ARCH SPECIFIC
 */

#if   defined(INTEL_SPINLOCKS) 

static inline int __spin_trylock(spinlock_t *lock)
{
	int oldval;

	asm volatile("xchgl %0,%1"
		: "=r" (oldval), "=m" (*lock)
		: "0" (0)
		: "memory");

	return oldval > 0 ? 0 : EBUSY;
}

static inline void __spin_unlock(spinlock_t *lock)
{
	asm volatile("":::"memory");
	*lock = 1;
}

static inline void __spin_lock_init(spinlock_t *lock)
{
	*lock = 1;
}

static inline int __spin_is_locked(spinlock_t *lock)
{
	return (*lock != 1);
}

#else
#error Need to implement spinlock code in spinlock.c
#endif

/*
 * OS SPECIFIC
 */

static void yield_cpu(void)
{
	struct timespec tm;

#ifdef USE_SCHED_YIELD
	sched_yield();
#else
	/* Linux will busy loop for delays < 2ms on real time tasks */
	tm.tv_sec = 0;
	tm.tv_nsec = 2000000L + 1;
	nanosleep(&tm, NULL);
#endif
}

static int this_is_smp(void)
{
	return 0;
}

/*
 * GENERIC
 */

static int smp_machine = 0;

static inline void __spin_lock(spinlock_t *lock)
{
	int ntries = 0;

	while(__spin_trylock(lock)) {
		while(__spin_is_locked(lock)) {
			if (smp_machine && ntries++ < MAX_BUSY_LOOPS)
				continue;
			yield_cpu();
		}
	}
}

static void __read_lock(tdb_rwlock_t *rwlock)
{
	int ntries = 0;

	while(1) {
		__spin_lock(&rwlock->lock);

		if (!(rwlock->count & RWLOCK_BIAS)) {
			rwlock->count++;
			__spin_unlock(&rwlock->lock);
			return;
		}
	
		__spin_unlock(&rwlock->lock);

		while(rwlock->count & RWLOCK_BIAS) {
			if (smp_machine && ntries++ < MAX_BUSY_LOOPS)
				continue;
			yield_cpu();
		}
	}
}

static void __write_lock(tdb_rwlock_t *rwlock)
{
	int ntries = 0;

	while(1) {
		__spin_lock(&rwlock->lock);

		if (rwlock->count == 0) {
			rwlock->count |= RWLOCK_BIAS;
			__spin_unlock(&rwlock->lock);
			return;
		}

		__spin_unlock(&rwlock->lock);

		while(rwlock->count != 0) {
			if (smp_machine && ntries++ < MAX_BUSY_LOOPS)
				continue;
			yield_cpu();
		}
	}
}

static void __write_unlock(tdb_rwlock_t *rwlock)
{
	__spin_lock(&rwlock->lock);

#ifdef DEBUG
	if (!(rwlock->count & RWLOCK_BIAS))
		fprintf(stderr, "bug: write_unlock\n");
#endif

	rwlock->count &= ~RWLOCK_BIAS;
	__spin_unlock(&rwlock->lock);
}

static void __read_unlock(tdb_rwlock_t *rwlock)
{
	__spin_lock(&rwlock->lock);

#ifdef DEBUG
	if (!rwlock->count)
		fprintf(stderr, "bug: read_unlock\n");

	if (rwlock->count & RWLOCK_BIAS)
		fprintf(stderr, "bug: read_unlock\n");
#endif

	rwlock->count--;
	__spin_unlock(&rwlock->lock);
}

/* TDB SPECIFIC */

/* lock a list in the database. list -1 is the alloc list */
int tdb_spinlock(TDB_CONTEXT *tdb, int list, int rw_type)
{
	tdb_rwlock_t *rwlocks;

	if (!tdb->map_ptr) return -1;
	rwlocks = (tdb_rwlock_t *)((char *)tdb->map_ptr + tdb->header.rwlocks);

	switch(rw_type) {
	case F_RDLCK:
		__read_lock(&rwlocks[list+1]);
		break;

	case F_WRLCK:
		__write_lock(&rwlocks[list+1]);
		break;

	default:
		return TDB_ERRCODE(TDB_ERR_LOCK, -1);
	}
	return 0;
}

/* unlock the database. */
int tdb_spinunlock(TDB_CONTEXT *tdb, int list, int rw_type)
{
	tdb_rwlock_t *rwlocks;

	if (!tdb->map_ptr) return -1;
	rwlocks = (tdb_rwlock_t *)((char *)tdb->map_ptr + tdb->header.rwlocks);

	switch(rw_type) {
	case F_RDLCK:
		__read_unlock(&rwlocks[list+1]);
		break;

	case F_WRLCK:
		__write_unlock(&rwlocks[list+1]);
		break;

	default:
		return TDB_ERRCODE(TDB_ERR_LOCK, -1);
	}

	return 0;
}

int tdb_create_rwlocks(int fd, unsigned int hash_size)
{
	unsigned size, i;
	tdb_rwlock_t *rwlocks;

	size = (hash_size + 1) * sizeof(tdb_rwlock_t);
	rwlocks = malloc(size);
	if (!rwlocks)
		return -1;

	for(i = 0; i < hash_size+1; i++) {
		__spin_lock_init(&rwlocks[i].lock);
		rwlocks[i].count = 0;
	}

	/* Write it out (appending to end) */
	if (write(fd, rwlocks, size) != size) {
		free(rwlocks);
		return -1;
	}
	smp_machine = this_is_smp();
	free(rwlocks);
	return 0;
}

int tdb_clear_spinlocks(TDB_CONTEXT *tdb)
{
	tdb_rwlock_t *rwlocks;
	unsigned i;

	if (tdb->header.rwlocks == 0) return 0;
	if (!tdb->map_ptr) return -1;

	/* We're mmapped here */
	rwlocks = (tdb_rwlock_t *)((char *)tdb->map_ptr + tdb->header.rwlocks);
	for(i = 0; i < tdb->header.hash_size+1; i++) {
		__spin_lock_init(&rwlocks[i].lock);
		rwlocks[i].count = 0;
	}
	return 0;
}
#else
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
#endif
