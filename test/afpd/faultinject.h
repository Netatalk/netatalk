/*
  Copyright (c) 2026 Andy Lemin (andylemin)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/

#ifndef FAULTINJECT_H
#define FAULTINJECT_H

#include <sys/types.h>   /* off_t */

/*!
 * @file
 * @brief Test-only fault-injection control block (see faultinject.c).
 *
 * Usage in a test (LD_PRELOAD interposition is global, so arm narrowly):
 *   fault_inject_reset();
 *   fi.open_armed = 1; fi.open_fail_after = 1; fi.open_errno = EMFILE;
 *   ... run only the operation under test ...
 *   fault_inject_reset();
 *
 * fail_after = N: the next N armed calls succeed, call N+1 fails once.
 * open_fail_flags = F: only opens whose flags include F count and can fail.
 */

/*! The countdown of one interposed call, with its count of armed calls */
struct fi_call {
    int armed;
    int fail_after;
    int errnum;
    int calls;              /*!< count of armed calls seen */
};

struct fault_inject {
    int open_armed;
    int open_fail_after;    /*!< succeed this many armed opens, then fail once */
    int open_errno;
    int open_calls;         /*!< count of armed open() calls seen */
    int open_last_flags;    /*!< flags arg of the most recent armed open() */
    int open_fail_flags;    /*!< opens without these flags pass through */
    int close_armed;
    int close_fail_after;
    int close_errno;
    struct fi_call read;    /*!< read() and pread() alike */
    int read_max;           /*!< read() and pread() read at most this */
    int pread_calls;        /*!< pread() calls, armed or not */
    int write_max;          /*!< a write() writes at most this many bytes */
    int write_zero;         /*!< the next write_zero write()s write nothing */
    struct fi_call lseek;
    struct fi_call asp_wrtcont;
    int asp_wrtcont_fill;   /*!< asp_wrtcont() hands back the bytes asked for */
    int fcntl_armed;
    int fcntl_fail_after;
    int fcntl_errno;
    /* Observe the lock calls a probe issues, so a test can assert HOW it probed
     * (count, range, type), not just the result.  Updated on every F_GETLK/F_SETLK
     * fcntl when fcntl_watch is set; reset by fault_inject_reset(). */
    int   fcntl_watch;       /*!< record lock-fcntl calls into the fields below */
    int   getlk_calls;       /*!< count of F_GETLK calls seen while watching */
    int   setlk_calls;       /*!< count of F_SETLK/F_SETLKW calls seen */
    int   lock_last_type;    /*!< l_type of the most recent watched lock fcntl */
    int   lock_last_cmd;     /*!< cmd of the most recent watched lock fcntl */
    off_t lock_last_start;   /*!< l_start of the most recent watched lock fcntl */
    off_t lock_last_len;     /*!< l_len   of the most recent watched lock fcntl */
    int   shrink_armed;      /*!< the next lock fcntl first cuts shrink_fd */
    int   shrink_fd;
    off_t shrink_to;         /*!< the length that cut leaves the file */
    int fchdir_armed;
    int fchdir_fail_after;
    int fchdir_errno;
    int dirfd_armed;
    int dirfd_fail_after;
    int dirfd_errno;
    int dirfd_calls;
    int dirfd_failed_fd;  /*!< descriptor from the injected call */
    struct fi_call splice_sock;
    struct fi_call splice_file;
    int pipe_size_limit;    /*!< F_SETPIPE_SZ above this fails with EPERM */
    int poll_urgent;        /*!< a wait for POLLPRI gets POLLPRI alone */
    int poll_waits;         /*!< count of 1-fd waits under poll_urgent */
    int poll_urgent_wakeups; /*!< count of POLLPRI waits answered */
    int pipe_budget_spent;  /*!< pipes stay at two pages and grow no more */
    struct fi_call pipe2;
    struct fi_call send;
    int send_max;           /*!< a send() sends at most this many bytes */
    struct fi_call sendfile;
    int sendfile_fake;      /*!< a sendfile() reports its count, moving none */
    int sendfile_eof_einval; /*!< a sendfile() at the file's end: EINVAL */
};

extern struct fault_inject fi;

void fault_inject_reset(void);

int faultinject_open_works(const char *scratch_path);

#endif /* FAULTINJECT_H */
