/*
 * Regression test for the interface list of getifacelist()
 *
 * Copyright (c) 2026 Andy Lemin (andylemin)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include <atalk/util.h>

int main(void)
{
    char **list = getifacelist();
    int empty = 0;
    printf("TAP version 13\n");

    if (list == NULL) {
        printf("ok 1 - getifacelist returns a name for every record # SKIP "
               "no interface list\n");
        printf("1..1\n");
        return 0;
    }

    /* a walk that steps inside a record reads an empty name there */
    for (char **name = list; *name != NULL; name++) {
        empty += **name == '\0';
    }

    freeifacelist(list);
    printf("%s 1 - getifacelist returns a name for every record\n",
           empty == 0 ? "ok" : "not ok");
    printf("1..1\n");
    return empty == 0 ? 0 : 1;
}
