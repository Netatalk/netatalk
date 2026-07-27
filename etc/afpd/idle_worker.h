/*
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

#ifndef IDLE_WORKER_H
#define IDLE_WORKER_H

/* Lock-free idle worker — handshake contract documented in idle_worker.c */

extern int  iw_init(void);
extern void iw_grant(void);
extern void iw_revoke(void);
extern void iw_revoke_signal_safe(void);
extern void iw_shutdown(void);
extern void iw_note_work(void);
extern int  iw_grant_active(void);
extern int  iw_is_active(void);
extern void iw_log_stats(void);

#endif /* IDLE_WORKER_H */
