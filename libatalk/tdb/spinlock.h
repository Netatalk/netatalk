#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <atalk/tdb.h>

#if 0
#define tdb_create_rwlocks(fd, hash_size) 0
#define tdb_spinlock(tdb, list, rw_type) (-1)
#define tdb_spinunlock(tdb, list, rw_type) (-1)
#else
int tdb_spinlock(TDB_CONTEXT *tdb, int list, int rw_type);
int tdb_spinunlock(TDB_CONTEXT *tdb, int list, int rw_type);
int tdb_create_rwlocks(int fd, unsigned int hash_size);
#endif
int tdb_clear_spinlocks(TDB_CONTEXT *tdb);

#endif
