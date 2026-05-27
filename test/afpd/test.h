/*
  Copyright (c) 2010 Frank Lahm <franklahm@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/

#ifndef TEST_H
#define TEST_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <atalk/cnid.h>
#include <atalk/directory.h>
#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/queue.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#include "afp_config.h"
#include "dircache.h"
#include "directory.h"
#include "hash.h"
#include "subtests.h"
#include "volume.h"

extern int test_output_tap;
extern int test_case_num;
extern FILE *test_report_stream;

static inline FILE *test_stream(void)
{
    return test_report_stream ? test_report_stream : stdout;
}

static inline void alignok(int len)
{
    int i = 1;

    if (len < 80) {
        i = 80 - len;
    }

    while (i--) {
        fprintf(test_stream(), " ");
    }
}

static inline void test_plan(int count)
{
    if (test_output_tap) {
        fprintf(test_stream(), "1..%d\n", count);
        fflush(test_stream());
    }
}

static inline void test_section(const char *title, const char *underline)
{
    if (!test_output_tap) {
        fprintf(test_stream(), "%s\n%s\n", title, underline);
    }
}

static inline void test_begin(const char *name)
{
    if (!test_output_tap) {
        int name_len;
        fprintf(test_stream(), "Testing: ");
        name_len = fprintf(test_stream(), "%s", name);
        fprintf(test_stream(), " ... ");

        if (name_len >= 0) {
            alignok(name_len);
        }
    }
}

static inline void test_ok(const char *name)
{
    if (test_output_tap) {
        fprintf(test_stream(), "ok %d - %s\n", ++test_case_num, name);
        fflush(test_stream());
    } else {
        fprintf(test_stream(), "[ok]\n");
    }
}

static inline void test_fail(const char *name, const char *file, int line)
{
    if (test_output_tap) {
        fprintf(test_stream(), "not ok %d - %s\n", ++test_case_num, name);
        fprintf(test_stream(), "# failed at %s:%d\n", file, line);
        fflush(test_stream());
    } else {
        fprintf(test_stream(), "[error]\n");
    }
}

static inline void test_fail_int(const char *name, int got, int expected,
                                 const char *file, int line)
{
    test_fail(name, file, line);

    if (test_output_tap) {
        fprintf(test_stream(), "# got: %d\n", got);
        fprintf(test_stream(), "# expected: %d\n", expected);
        fflush(test_stream());
    }
}

static inline void test_abort(void)
{
    if (test_output_tap) {
        fprintf(test_stream(), "Bail out! stopping after failed assertion\n");
        fflush(test_stream());
    }
}

#define TEST(a) \
    do {                               \
        test_begin(#a);                \
        a;                             \
        test_ok(#a);                   \
    } while (0)

#define TEST_int(a, b) \
    do {                                           \
        test_begin(#a);                            \
        if ((reti = (a)) != b) {                   \
            test_fail_int(#a, reti, b, __FILE__,   \
                          __LINE__);               \
            test_abort();                           \
            exit(1);                               \
        } else {                                   \
            test_ok(#a);                           \
        }                                          \
    } while (0)

#define TEST_expr(a, b)                           \
    do {                                          \
        test_begin(#a);                           \
        a;                                        \
        if (b) {                                  \
            test_ok(#a);                          \
        } else {                                  \
            test_fail(#a, __FILE__, __LINE__);    \
            test_abort();                         \
            exit(1);                              \
        }                                         \
    } while (0)
#endif  /* TEST_H */
