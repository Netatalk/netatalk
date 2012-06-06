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

#include "dd.h"

static const char *neststrings[] = {
    "",
    "    ",
    "        ",
    "            ",
    "                ",
    "                    ",
    "                        "
};

void *_dd_add_obj(dd_t *dd, void *talloc_chunk, void *obj, size_t size)
{
    AFP_ASSERT(talloc_chunk);
    
    if (dd->dd_talloc_array == NULL) {
        dd->dd_talloc_array = talloc_array(dd, void *, 1);
    } else {
        dd->dd_talloc_array = talloc_realloc(dd, dd->dd_talloc_array, void *, talloc_array_length(dd->dd_talloc_array) + 1);
    }

    memcpy(talloc_chunk, obj, size);
    dd->dd_talloc_array[talloc_array_length(dd->dd_talloc_array) - 1] = talloc_chunk;

    return 0;
}

static int dd_dump(dd_t *dd, int nestinglevel)
{
    const char *type;

    printf("%sArray(#%d): {\n", neststrings[nestinglevel], talloc_array_length(dd->dd_talloc_array));

    for (int n = 0; n < talloc_array_length(dd->dd_talloc_array); n++) {

        type = talloc_get_name(dd->dd_talloc_array[n]);

        if (STRCMP(type, ==, "int64_t")) {
            int64_t i;
            memcpy(&i, dd->dd_talloc_array[n], sizeof(int64_t));
            printf("%s%d:\t%" PRId64 "\n", neststrings[nestinglevel + 1], n, i);
        } else if (STRCMP(type, ==, "bstring")) {
            bstring b;
            memcpy(&b, dd->dd_talloc_array[n], sizeof(bstring));
            printf("%s%d:\t%s\n", neststrings[nestinglevel + 1], n, bdata(b));
        } else if (STRCMP(type, ==, "_Bool")) {
            bool bl;
            memcpy(&bl, dd->dd_talloc_array[n], sizeof(bool));
            printf("%s%d:\t%s\n", neststrings[nestinglevel + 1], n, bl ? "true" : "false");
        } else if (STRCMP(type, ==, "dd_t")) {
            dd_t *nested;
            memcpy(&nested, dd->dd_talloc_array[n], sizeof(dd_t *));
            dd_dump(nested, nestinglevel + 1);
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
    dd_add_obj(dd, &i, int64_t);

    bstring str = bfromcstr("hello world");
    dd_add_obj(dd, &str, bstring);

    bool b = true;
    dd_add_obj(dd, &b, bool);

    b = false;
    dd_add_obj(dd, &b, bool);

    i = 1;
    dd_add_obj(dd, &i, int64_t);

    /* add a nested array */
    dd_t *nested = talloc_zero(dd, dd_t);
    dd_add_obj(nested, &i, int64_t);
    dd_add_obj(nested, &str, bstring);
    dd_add_obj(dd, &nested, dd_t);

    dd_dump(dd, 0);

    talloc_free(mem_ctx);
    return 0;
}
#endif
