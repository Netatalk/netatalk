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

#ifndef SUBTESTS_LOCK_H
#define SUBTESTS_LOCK_H

#include <atalk/volume.h>

extern int utest_faultinject_selftest(const struct vol *vol);
extern int utest_openfork_no_fd_leak(const struct vol *vol);
extern int utest_shared_adouble_refcount_balance(const struct vol *vol);
extern int utest_adclose_underflow_aborts(const struct vol *vol);
extern int utest_ro_retry_strips_destructive_flags(const struct vol *vol);

#endif /* SUBTESTS_LOCK_H */
