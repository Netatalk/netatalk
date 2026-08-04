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

#endif /* PEER_LOCK_H */
