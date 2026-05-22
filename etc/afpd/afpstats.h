/*
 * Copyright (c) 2013 Frank Lahm <franklahm@gmail.com>
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

#ifndef AFPD_AFPSTATS_H
#define AFPD_AFPSTATS_H

#include <stdbool.h>
#include <sys/types.h>

#include <atalk/server_child.h>

extern int afpstats_init(server_child_t *children, const char *sock_path,
                         bool set_group, gid_t gid);
extern void afpstats_handle_accept(int listen_fd);

#endif
