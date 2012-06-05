/*
  Copyright (c) 2012 Frank Lahm <franklahm@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>

#include <atalk/errchk.h>
#include <atalk/util.h>
#include <atalk/logger.h>
#include <atalk/talloc.h>

#include "dd.h"

static const char *neststrings[] = {
    "",
    "    ",
    "        ",
    "            "
};

static int dd_add_elem(dd_t *dd, dde_type_e type, void *val, size_t size)
{
    if (dd->dd_count == 0) {
        dd->dd_elem = talloc_array(dd, dde_t *, 1);
    } else {
        dd->dd_elem = talloc_realloc(dd, dd->dd_elem, dde_t *, dd->dd_count + 1);
    }

    if (dd->dd_elem == NULL) {
        LOG(logtype_default, log_error, "Allocation error");
        return -1;
    }

    dd->dd_elem[dd->dd_count] = talloc(dd->dd_elem, dde_t);
    dd->dd_elem[dd->dd_count]->dde_type = type;
    dd->dd_elem[dd->dd_count]->dde_val = talloc_memdup(dd->dd_elem, val, size);

    dd->dd_count++;

    return 0;
}

static int dd_dump(dd_t *dd, int nestinglevel)
{
    printf("%sArray(#%d): {\n", neststrings[nestinglevel], dd->dd_count);
    for (int n = 0; n < dd->dd_count; n++) {
        switch (dd->dd_elem[n]->dde_type) {
        case DDT_INT64: {
            int i;
            memcpy(&i, dd->dd_elem[n]->dde_val, sizeof(int));
            printf("%s%d:\t%d\n", neststrings[nestinglevel + 1], n, i);
            break;
        }
        case DDT_STRING:
            printf("%s%d:\t%s\n", neststrings[nestinglevel + 1], n, dd->dd_elem[n]->dde_val);
            break;
        case DDT_BOOL: {
            bool b;
            memcpy(&b, dd->dd_elem[n]->dde_val, sizeof(bool));
            printf("%s%d:\t%s\n", neststrings[nestinglevel + 1], n, b ? "true" : "false");
            break;
        }
        case DDT_ARRAY:
            dd_dump(dd->dd_elem[n]->dde_val, nestinglevel + 1);
            break;
        default:
            LOG(logtype_default, log_error, "Unknown type");
            break;
        }
    }
    printf("%s}\n", neststrings[nestinglevel]);
}

#ifdef SPOT_TEST_MAIN
#include <stdarg.h>

int main(int argc, char **argv)
{
    TALLOC_CTX *mem_ctx = talloc_new(NULL);
    dd_t *dd = talloc_zero(mem_ctx, dd_t);
    int i;

    set_processname("spot");
    setuplog("default:info", "/dev/tty");

    LOG(logtype_default, log_info, "Start");

    i = 2;
    dd_add_elem(dd, DDT_INT64, &i, sizeof(int));

    i = 3;
    dd_add_elem(dd, DDT_INT64, &i, sizeof(int));

    char *str = "hello world";
    dd_add_elem(dd, DDT_STRING, str, strlen(str) + 1);

    bool b = true;
    dd_add_elem(dd, DDT_BOOL, &b, sizeof(bool));

    b = false;
    dd_add_elem(dd, DDT_BOOL, &b, sizeof(bool));

    i = 1;
    dd_add_elem(dd, DDT_INT64, &i, sizeof(int));

    /* add a nested array */
    dd_t *nested = talloc_zero(dd, dd_t);

    dd_add_elem(nested, DDT_INT64, &i, sizeof(int));

    dd_add_elem(dd, DDT_ARRAY, nested, sizeof(dd_t));

    dd_dump(dd, 0);

    talloc_free(mem_ctx);
    return 0;
}
#endif
