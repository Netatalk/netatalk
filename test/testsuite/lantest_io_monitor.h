/*
 * Copyright (c) 2025, Andy Lemin (andylemin)
 * Credits; Based on work by Netatalk contributors
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

#ifndef LANTEST_IO_MONITOR_H
#define LANTEST_IO_MONITOR_H

#include <stdint.h>
#include <sys/types.h>

/* Define PID_MAX if not available from system headers */
#ifndef PID_MAX
/* Conservative default maximum PID value */
#define PID_MAX 99999
#endif

/* Global Debug flag - controlled by -b option in lantest.c */
extern uint8_t Debug;

#ifdef __linux__

/* Process filtering configuration for finding Netatalk daemons.
 * Supports two modes: filter by process UID ownership (for afpd which drops privileges)
 * or filter by cmdline -u argument (for cnid_dbd which runs as root). */
typedef struct {
    const char *process_name;
    const char *username;
    int32_t filter_by_cmdline;  /* 0 = filter by UID ownership, 1 = filter by cmdline -u arg */
    uid_t target_uid;       /* For ownership filtering */
} ProcessFilter;

/* Container for discovered process IDs during /proc_io scanning.
 * Stores up to 10 matching PIDs to detect multiple instances.
 * count=0 means no match, count=1 is ideal, count>1 indicates duplicates. */
typedef struct {
    pid_t pids[10];
    int32_t count;
} ProcessList;

/* External variables for IO monitoring */
extern uint8_t io_monitoring_enabled;
extern pid_t afpd_pid;
extern pid_t cnid_dbd_pid;
extern uint64_t afpd_start_reads, afpd_start_writes;
extern uint64_t cnid_start_reads, cnid_start_writes;
extern uint64_t afpd_end_reads, afpd_end_writes;
extern uint64_t cnid_end_reads, cnid_end_writes;

/* Constants for capture_io_values() */
#define TEST_START 1
#define TEST_STOP  0

/* Function declarations */
int32_t check_proc_io_availability(void);
pid_t find_process_pid(const char *process_name, const char *username,
                       int32_t filter_by_cmdline);
void capture_io_values(int32_t is_start);
uint64_t iodiff_io(pid_t pid, int32_t is_write);

/* Initialization function */
void init_io_monitoring(const char *username);

#endif /* __linux__ */

#endif /* LANTEST_IO_MONITOR_H */