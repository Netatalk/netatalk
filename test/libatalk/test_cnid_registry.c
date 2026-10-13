/*
 * Copyright (c) 2026 Daniel Markstedt <daniel@mindani.net>
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

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <atalk/cnid.h>
#include <atalk/volume.h>

static struct _cnid_db first_db;
static struct _cnid_db second_db;
static int rejected_calls;

static struct _cnid_db *first_open(struct cnid_open_args *args)
{
    assert(args->cnid_args_vol != NULL);
    return &first_db;
}

static struct _cnid_db *second_open(struct cnid_open_args *args)
{
    assert(args->cnid_args_vol != NULL);
    return &second_db;
}

static struct _cnid_db *rejected_open(struct cnid_open_args *args)
{
    (void)args;
    rejected_calls++;
    return NULL;
}

int main(void)
{
    static cnid_module first = {.name = "RegistryFirst", .cnid_open = first_open};
    static cnid_module second = {.name = "RegistrySecond", .cnid_open = second_open};
    static cnid_module duplicate = {
        .name = "rEGISTRYfIRST", .cnid_open = rejected_open
    };
    struct vol vol = {.v_path = "/registry-test"};
    assert(!cnid_scheme_registered("RegistryFirst"));
    assert(!cnid_scheme_registered("unregistered"));
    cnid_register(&first);
    cnid_register(&second);
    assert(cnid_scheme_registered("REGISTRYFIRST"));
    assert(cnid_scheme_registered("registrysecond"));
    assert(cnid_open(&vol, "registryfirst", 0) == &first_db);
    assert(cnid_open(&vol, "REGISTRYSECOND", CNID_FLAG_NODEV) == &second_db);
    assert(second_db.cnid_db_flags & CNID_FLAG_NODEV);
    /* Same pointer and a distinct module with the same name are both rejected. */
    cnid_register(&first);
    cnid_register(&second);
    cnid_register(&duplicate);
    assert(cnid_open(&vol, "RegistryFirst", 0) == &first_db);
    assert(cnid_open(&vol, "RegistrySecond", 0) == &second_db);
    assert(rejected_calls == 0);
    assert(!cnid_scheme_registered("unregistered"));
    puts("CNID registry tests passed");
    return 0;
}
