/*
 * Copyright (c) 2026 Andy Lemin (@andylemin)
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

#ifndef IDLE_WORKER_H
#define IDLE_WORKER_H

extern int  idle_worker_init(void);
extern void idle_worker_start(void);
extern void idle_worker_stop(void);
extern void idle_worker_stop_signal_safe(void);
extern void idle_worker_shutdown(void);
extern int  idle_worker_is_active(void);
extern void idle_worker_log_stats(void);

#endif /* IDLE_WORKER_H */
