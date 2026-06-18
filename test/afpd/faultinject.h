/*
  Copyright (c) 2026 andylemin

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
 */

struct fault_inject {
    int open_armed;
    int open_fail_after;    /*!< succeed this many armed opens, then fail once */
    int open_errno;
    int open_calls;         /*!< count of armed open() calls seen */
    int open_last_flags;    /*!< flags arg of the most recent armed open() */
    int close_armed;
    int close_fail_after;
    int close_errno;
    int fcntl_armed;
    int fcntl_fail_after;
    int fcntl_errno;
    int fchdir_armed;
    int fchdir_fail_after;
    int fchdir_errno;
};

extern struct fault_inject fi;

void fault_inject_reset(void);

int faultinject_open_works(const char *scratch_path);

#endif /* FAULTINJECT_H */
