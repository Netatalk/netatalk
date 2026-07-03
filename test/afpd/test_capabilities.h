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

#ifndef TEST_CAPABILITIES_H
#define TEST_CAPABILITIES_H

#include <atalk/volume.h>

enum test_capability {
    TEST_CAP_FAULT_INJECT,   /* LD_PRELOAD libc interposition reaches libatalk */
    TEST_CAP_EA_SYS,         /* the test filesystem supports user xattrs (ea=sys) */
};

int test_capability(enum test_capability cap, const struct vol *vol);

const char *test_capability_name(enum test_capability cap);

#endif /* TEST_CAPABILITIES_H */
