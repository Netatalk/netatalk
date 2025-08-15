/*
 * Copyright (c) 2025, Andy Lemin (andylemin@github.com)
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

/* Global Debug flag - controlled by -b option in lantest.c */
extern int Debug;

#ifdef __linux__

#include <sys/types.h>
#include <stdint.h>

/* Structure to hold process filtering configuration */
typedef struct {
    const char *process_name;
    const char *username;
    int filter_by_cmdline;  /* 0 = filter by UID ownership, 1 = filter by cmdline -u arg */
    uid_t target_uid;       /* For ownership filtering */
} ProcessFilter;

/* Structure to hold discovered process information */
typedef struct {
    pid_t pids[10];
    int count;
} ProcessList;

/* External variables for IO monitoring */
extern int io_monitoring_enabled;
extern pid_t afpd_pid;
extern pid_t cnid_dbd_pid;
extern unsigned long long afpd_start_reads, afpd_start_writes;
extern unsigned long long cnid_start_reads, cnid_start_writes;
extern unsigned long long afpd_end_reads, afpd_end_writes;
extern unsigned long long cnid_end_reads, cnid_end_writes;

/* Constants for capture_io_values() */
#define TEST_START 1
#define TEST_STOP  0

/* Function declarations */
int check_proc_io_availability(void);
pid_t find_process_pid(const char *process_name, const char *username,
                       int filter_by_cmdline);
void capture_io_values(int is_start);
unsigned long long iodiff_io(pid_t pid, int is_write);

/* Initialization function */
void init_io_monitoring(const char *username);

#endif /* __linux__ */

#endif /* LANTEST_IO_MONITOR_H */