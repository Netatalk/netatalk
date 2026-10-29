/*!
 * @file
 * @brief Peer process helpers for component test file locks
 */

#ifndef PEER_LOCK_H
#define PEER_LOCK_H

#include <sys/types.h>

/* A forked peer holding a byte-range lock on a path until released. */
struct peer {
    pid_t pid;
    int   to_child;      /* parent writes 1 byte here to tell the peer to exit */
    int   from_child;    /* peer writes 1 byte here once the lock is held */
};

extern int peer_hold_lock(struct peer *p, const char *path, short type,
                          off_t start, off_t len);
extern void peer_release(struct peer *p);
extern int peer_try_lock(const char *path, short type, off_t start,
                         off_t len);

/* A kernel lock a forked child tries on a range, and whether the range is
 * expected free. */
struct range_probe {
    short type;          /* F_RDLCK or F_WRLCK */
    off_t off;           /* first byte of the range */
    off_t len;           /* length of the range */
    int   free;          /* 1 when the child should get the lock, 0 when not */
};

extern int probe_ranges(const char *path, const struct range_probe *probes,
                        int count, int *seen);

#endif /* PEER_LOCK_H */
