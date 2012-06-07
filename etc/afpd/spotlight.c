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
#include <inttypes.h>

#include <atalk/errchk.h>
#include <atalk/util.h>
#include <atalk/logger.h>
#include <atalk/talloc.h>
#include <atalk/bstrlib.h>

#include "spotlight.h"

#ifdef SPOT_TEST_MAIN

static const char *neststrings[] = {
    "",
    "    ",
    "        ",
    "            ",
    "                ",
    "                    ",
    "                        "
};

static int dd_dump(DALLOC_CTX *dd, int nestinglevel)
{
    const char *type;

    printf("%sArray(#%d): {\n", neststrings[nestinglevel], talloc_array_length(dd->dd_talloc_array));

    for (int n = 0; n < talloc_array_length(dd->dd_talloc_array); n++) {

        type = talloc_get_name(dd->dd_talloc_array[n]);

        if (STRCMP(type, ==, "int64_t")) {
            int64_t i;
            memcpy(&i, dd->dd_talloc_array[n], sizeof(int64_t));
            printf("%s%d:\t%" PRId64 "\n", neststrings[nestinglevel + 1], n, i);
        } else if (STRCMP(type, ==, "uint32_t")) {
            uint32_t i;
            memcpy(&i, dd->dd_talloc_array[n], sizeof(uint32_t));
            printf("%s%d:\t%" PRIu32 "\n", neststrings[nestinglevel + 1], n, i);
        } else if (STRCMP(type, ==, "char *")) {
            char *s;
            memcpy(&s, dd->dd_talloc_array[n], sizeof(char *));
            printf("%s%d:\t%s\n", neststrings[nestinglevel + 1], n, s);
        } else if (STRCMP(type, ==, "_Bool")) {
            bool bl;
            memcpy(&bl, dd->dd_talloc_array[n], sizeof(bool));
            printf("%s%d:\t%s\n", neststrings[nestinglevel + 1], n, bl ? "true" : "false");
        } else if (STRCMP(type, ==, "dd_t")) {
            DALLOC_CTX *nested;
            memcpy(&nested, dd->dd_talloc_array[n], sizeof(DALLOC_CTX *));
            dd_dump(nested, nestinglevel + 1);
        } else if (STRCMP(type, ==, "cnid_array_t")) {
            cnid_array_t *cnids;
            memcpy(&cnids, dd->dd_talloc_array[n], sizeof(cnid_array_t *));
            printf("%s%d:\tunkn1: %" PRIu16 ", unkn2: %" PRIu32,
                   neststrings[nestinglevel + 1], n, cnids->ca_unkn1, cnids->ca_unkn2);
            if (cnids->ca_cnids)
                dd_dump(cnids->ca_cnids, nestinglevel + 1);
        }
    }
    printf("%s}\n", neststrings[nestinglevel]);
}

#include <stdarg.h>

int main(int argc, char **argv)
{
    TALLOC_CTX *mem_ctx = talloc_new(NULL);
    DALLOC_CTX *dd = talloc_zero(mem_ctx, DALLOC_CTX);
    int64_t i;

    set_processname("spot");
    setuplog("default:info", "/dev/tty");

    LOG(logtype_default, log_info, "Start");

    i = 2;
    dalloc_add(dd, &i, int64_t);

    i = 1;
    dalloc_add(dd, &i, int64_t);


    char *str = talloc_strdup(dd, "hello world");
    dalloc_add(dd, &str, char *);

    bool b = true;
    dalloc_add(dd, &b, bool);

    b = false;
    dalloc_add(dd, &b, bool);


    /* add a nested array */
    DALLOC_CTX *nested = talloc_zero(dd, DALLOC_CTX);
    i = 3;
    dalloc_add(nested, &i, int64_t);
    dalloc_add(dd, &nested, DALLOC_CTX);

    /* test a CNID array */
    uint32_t id = 16;
    cnid_array_t *cnids = talloc_zero(dd, cnid_array_t);

    cnids->ca_cnids = talloc_zero(cnids, DALLOC_CTX);

    cnids->ca_unkn1 = 1;
    cnids->ca_unkn2 = 2;

    dalloc_add(cnids->ca_cnids, &id, uint32_t);
    dalloc_add(dd, &cnids, cnid_array_t);

    dd_dump(dd, 0);

    talloc_free(mem_ctx);
    return 0;
}
#endif
