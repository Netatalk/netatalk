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

/*
 * lantest_io_monitor.c - Linux I/O monitoring for Netatalk performance testing
 *
 * Monitors system call I/O statistics for afpd and cnid_dbd processes during tests.
 * Uses /proc_io filesystem (a secondary mount of proc) to track read/write syscalls.
 * Automatically discovers target processes by name and user, handling both privilege-dropped
 * processes (afpd) and root processes with user arguments (cnid_dbd).
 * Provides before/after test IO metrics to measure actual filesystem activity during AFP operations.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __linux__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <stdint.h>

#include "lantest_io_monitor.h"

#include <limits.h>  /* For PATH_MAX */

/* Global variables for IO monitoring */
uint8_t io_monitoring_enabled = 0;
pid_t afpd_pid = 0;
pid_t cnid_dbd_pid = 0;
uint64_t afpd_start_reads = 0, afpd_start_writes = 0;
uint64_t cnid_start_reads = 0, cnid_start_writes = 0;
uint64_t afpd_end_reads = 0, afpd_end_writes = 0;
uint64_t cnid_end_reads = 0, cnid_end_writes = 0;

/* Static helper function prototypes */
static pid_t safe_parse_pid(const char *pidstr);
static int32_t init_process_filter(ProcessFilter *filter,
                                   const char *process_name,
                                   const char *username, int32_t filter_by_cmdline);
static int32_t check_process_name_match(const char *pid_dir,
                                        const char *target_name);
static int32_t check_cmdline_filter(const char *pid_dir, const char *username);
static int32_t check_uid_filter(const char *pid_dir, uid_t target_uid);
static int32_t process_proc_entry(const char *pid_dir,
                                  const ProcessFilter *filter,
                                  ProcessList *found);
static void report_multiple_pids(const ProcessFilter *filter,
                                 const ProcessList *found);

/* Helper: Show warning about /proc_io availability */
static void show_proc_io_warning(const char *specific_issue)
{
    static int32_t warning_shown = 0;

    if (!warning_shown) {
        fprintf(stderr,
                "WARNING: %s. IO Monitoring disabled.\n"
                "To enable IO monitoring, mount proc filesystem at /proc_io. "
                "Ensure /proc_io mounted with 'gid' matching group of user running afp_lantest.\n"
                "Eg, as root:\n"
                "  mkdir -p /proc_io && mount -t proc -o hidepid=0,gid=0 proc /proc_io\n",
                specific_issue);
        warning_shown = 1;
    }
}

/* Helper: Check if /proc_io is available and show warning if not */
int32_t check_proc_io_availability(void)
{
    struct stat st;

    /* Check if /proc_io exists */
    if (stat("/proc_io", &st) != 0) {
        show_proc_io_warning("/proc_io directory not found");
        io_monitoring_enabled = 0;
        return 0;
    }

    /* /proc_io exists, now check if it appears to be properly mounted */
    DIR *proc_dir = opendir("/proc_io");

    if (!proc_dir) {
        show_proc_io_warning("Cannot open /proc_io directory");
        io_monitoring_enabled = 0;
        return 0;
    }

    /* Count entries in /proc_io to check if it's properly mounted */
    struct dirent *entry;
    int32_t entry_count = 0;

    while ((entry = readdir(proc_dir)) != NULL && entry_count < 5) {
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            entry_count++;
        }
    }

    closedir(proc_dir);

    /* If /proc_io appears to be empty or nearly empty, it's likely not properly mounted */
    if (entry_count < 3) {
        char message[256];
        snprintf(message, sizeof(message),
                 "/proc_io directory appears to be empty (found %d entries)",
                 entry_count);
        show_proc_io_warning(message);
        io_monitoring_enabled = 0;
        return 0;
    }

    return 1;
}

/* Helper: Initialize process filter configuration */
static int32_t init_process_filter(ProcessFilter *filter,
                                   const char *process_name,
                                   const char *username, int32_t filter_by_cmdline)
{
    filter->process_name = process_name;
    filter->username = username;
    filter->filter_by_cmdline = filter_by_cmdline;

    if (!filter_by_cmdline) {
        /* Convert username to UID for ownership-based filtering */
        struct passwd *pwd = getpwnam(username);

        if (!pwd) {
            fprintf(stderr, "Error: Unable to find UID for user '%s'\n", username);
            return -1;
        }

        filter->target_uid = pwd->pw_uid;
        fprintf(stdout, "Looking for %s processes owned by user '%s' (UID: %u)\n",
                process_name, username, filter->target_uid);
    } else {
        fprintf(stdout, "Looking for %s processes with -u %s in command line\n",
                process_name, username);
    }

    return 0;
}

/* Helper: Check if process name matches target_name */
static int32_t check_process_name_match(const char *pid_dir,
                                        const char *target_name)
{
    char comm_path[256];
    char comm_name[256];
    FILE *comm_file;
    snprintf(comm_path, sizeof(comm_path), "/proc_io/%s/comm", pid_dir);
    comm_file = fopen(comm_path, "r");

    if (!comm_file) {
        fprintf(stderr, "Could not open %s\n", comm_path);
        return 0;
    }

    int32_t matches = 0;

    if (fgets(comm_name, sizeof(comm_name), comm_file)) {
        comm_name[strcspn(comm_name, "\n")] = '\0';  /* Remove trailing newline */
        matches = (strcmp(comm_name, target_name) == 0);
    }

    fclose(comm_file);
    return matches;
}

/* Helper: Check if process matches cmdline filter (-u username) */
static int32_t check_cmdline_filter(const char *pid_dir, const char *username)
{
    char cmdline_path[256];
    char cmdline_buffer[1024];
    FILE *cmdline_file;
    snprintf(cmdline_path, sizeof(cmdline_path), "/proc_io/%s/cmdline", pid_dir);
    cmdline_file = fopen(cmdline_path, "r");

    if (!cmdline_file) {
        fprintf(stderr, "Could not open %s\n", cmdline_path);
        return 0;
    }

    size_t bytes_read = fread(cmdline_buffer, 1, sizeof(cmdline_buffer) - 1,
                              cmdline_file);
    cmdline_buffer[bytes_read] = '\0';
    fclose(cmdline_file);

    if (Debug) {
        fprintf(stderr, "DEBUG: PID %s cmdline: ", pid_dir);

        for (size_t i = 0; i < bytes_read; i++) {
            if (cmdline_buffer[i] == '\0') {
                fprintf(stderr, " ");
            } else {
                fprintf(stderr, "%c", cmdline_buffer[i]);
            }
        }
    }

    /* Parse null-separated command line arguments */
    char *arg = cmdline_buffer;
    char *end = cmdline_buffer + bytes_read;

    while (arg < end) {
        if (strcmp(arg, "-u") == 0) {
            arg += strlen(arg) + 1;  /* Move to next argument */

            if (arg < end && strcmp(arg, username) == 0) {
                if (Debug) {
                    fprintf(stderr, "DEBUG: Found matching -u %s in cmdline\n", username);
                }

                return 1;  /* Found matching -u username */
            }
        }

        arg += strlen(arg) + 1;
    }

    if (Debug) {
        fprintf(stderr, "DEBUG: No matching -u %s in cmdline\n", username);
    }

    return 0;
}

/* Helper: Check if process matches UID filter (ownership) */
static int32_t check_uid_filter(const char *pid_dir, uid_t target_uid)
{
    char status_path[256];
    char status_line[256];
    FILE *status_file;
    snprintf(status_path, sizeof(status_path), "/proc_io/%s/status", pid_dir);
    status_file = fopen(status_path, "r");

    if (!status_file) {
        fprintf(stderr, "DEBUG: Failed to open %s\n", status_path);
        return 0;
    }

    int32_t matches = 0;

    while (fgets(status_line, sizeof(status_line), status_file)) {
        if (strncmp(status_line, "Uid:", 4) == 0) {
            uid_t real_uid, effective_uid;

            if (sscanf(status_line, "Uid:\t%u\t%u", &real_uid, &effective_uid) == 2) {
                /* Check if either real or effective UID matches */
                matches = (real_uid == target_uid || effective_uid == target_uid);
            }

            break;
        }
    }

    fclose(status_file);
    return matches;
}

/* Helper: Process a single /proc directory entry */
static int32_t process_proc_entry(const char *pid_dir,
                                  const ProcessFilter *filter,
                                  ProcessList *found)
{
    /* Skip non-numeric entries (must be PID directories) */
    if (strspn(pid_dir, "0123456789") != strlen(pid_dir)) {
        return 0;
    }

    /* Check if process name matches */
    if (!check_process_name_match(pid_dir, filter->process_name)) {
        return 0;
    }

    /* Apply appropriate filter based on configuration */
    int32_t process_matches = 0;

    if (filter->filter_by_cmdline) {
        process_matches = check_cmdline_filter(pid_dir, filter->username);
    } else {
        process_matches = check_uid_filter(pid_dir, filter->target_uid);
    }

    /* Add to found list if matches */
    if (process_matches && found->count < 10) {
        pid_t pid = safe_parse_pid(pid_dir);

        if (pid > 0) {
            found->pids[found->count++] = pid;
            return 1;
        }
    }

    return 0;
}

/* Helper: Report multiple PIDs found */
static void report_multiple_pids(const ProcessFilter *filter,
                                 const ProcessList *found)
{
    if (filter->filter_by_cmdline) {
        fprintf(stderr, "Warning: Multiple %s processes with -u %s found (",
                filter->process_name, filter->username);
    } else {
        fprintf(stderr, "Warning: Multiple %s processes owned by %s found (",
                filter->process_name, filter->username);
    }

    for (int32_t i = 0; i < found->count; i++) {
        fprintf(stderr, "%d", found->pids[i]);

        if (i < found->count - 1) {
            fprintf(stderr, ", ");
        }
    }

    fprintf(stderr, "), using first: %d\n", found->pids[0]);
}

/* Main Netatalk process find function for IO monitoring */
pid_t find_process_pid(const char *process_name, const char *username,
                       int32_t filter_by_cmdline)
{
    /* Check prerequisites */
    if (!io_monitoring_enabled) {
        return 0;
    }

    /* Initialize filter configuration */
    ProcessFilter filter;

    if (init_process_filter(&filter, process_name, username,
                            filter_by_cmdline) < 0) {
        if (Debug) {
            fprintf(stderr, "DEBUG: Failed to initialize process filter\n");
        }

        return 0;
    }

    /* Open /proc_io directory */
    DIR *proc_dir = opendir("/proc_io");

    if (!proc_dir) {
        if (Debug) {
            fprintf(stderr, "DEBUG: Failed to open /proc_io directory (errno: %d)\n",
                    errno);
        }

        return 0;
    }

    /* Scan directory entries and collect matching PIDs */
    ProcessList found = { .count = 0 };
    struct dirent *entry;
    int32_t entries_checked = 0;
    int32_t total_entries = 0;

    while ((entry = readdir(proc_dir)) != NULL && found.count < 10) {
        total_entries++;

        /* Only process numeric entries (PID directories) */
        if (strspn(entry->d_name, "0123456789") == strlen(entry->d_name)) {
            entries_checked++;
            process_proc_entry(entry->d_name, &filter, &found);
        }
    }

    closedir(proc_dir);

    if (Debug) {
        fprintf(stderr,
                "DEBUG: /proc_io scan complete - total entries: %d, numeric PID directories: %d, matches found: %d\n",
                total_entries, entries_checked, found.count);
    }

    /* Handle results */
    if (found.count == 0) {
        fprintf(stdout, "No matching processes found\n");
        return 0;
    }

    if (found.count > 1) {
        report_multiple_pids(&filter, &found);
    }

    return found.pids[0];
}

/* Read IO statistics from /proc_io/<pid>/io file.
 * syscr: cumulative count of read system calls (read(), pread(), readv(), etc.)
 * syscw: cumulative count of write system calls (write(), pwrite(), writev(), etc.) */
static int32_t read_proc_io(pid_t pid, uint64_t *read_ops,
                            uint64_t *write_ops)
{
    FILE *io_file;
    char io_path[256];
    char line[256];
    int32_t found_read = 0, found_write = 0;
    *read_ops = 0;
    *write_ops = 0;
    snprintf(io_path, sizeof(io_path), "/proc_io/%d/io", pid);
    io_file = fopen(io_path, "r");

    if (!io_file) {
        fprintf(stderr, "Cannot open %s (errno: %d)\n", io_path, errno);
        return 0;
    }

    while (fgets(line, sizeof(line), io_file) && (!found_read || !found_write)) {
        if (sscanf(line, "syscr: %llu", read_ops) == 1) {
            found_read = 1;
        } else if (sscanf(line, "syscw: %llu", write_ops) == 1) {
            found_write = 1;
        }
    }

    fclose(io_file);

    if (Debug) {
        fprintf(stderr, "DEBUG: IO result (%d) - syscr: %llu, syscw: %llu\n", pid,
                *read_ops, *write_ops);
    }

    return (found_read && found_write) ? 0 : -1;
}

/* Capture IO values - consolidated function for start and stop */
void capture_io_values(int32_t is_start)
{
    if (!io_monitoring_enabled) {
        return;
    }

    /* Process afpd_pid */
    if (afpd_pid > 0) {
        uint64_t *reads = is_start ? &afpd_start_reads : &afpd_end_reads;
        uint64_t *writes = is_start ? &afpd_start_writes : &afpd_end_writes;

        if (read_proc_io(afpd_pid, reads, writes) != 0) {
            *reads = *writes = 0;
        }
    }

    /* Process cnid_dbd_pid */
    if (cnid_dbd_pid > 0) {
        uint64_t *reads = is_start ? &cnid_start_reads : &cnid_end_reads;
        uint64_t *writes = is_start ? &cnid_start_writes : &cnid_end_writes;

        if (read_proc_io(cnid_dbd_pid, reads, writes) != 0) {
            *reads = *writes = 0;
        }
    }
}

/* Get IO delta between stored cumulative counts - consolidated for read and write */
uint64_t iodiff_io(pid_t pid, int32_t is_write)
{
    uint64_t start_val, end_val;

    if (!io_monitoring_enabled || pid == 0) {
        return 0;  /* Return 0 instead of -1 for unsigned return type */
    }

    if (pid == afpd_pid) {
        start_val = is_write ? afpd_start_writes : afpd_start_reads;
        end_val = is_write ? afpd_end_writes : afpd_end_reads;
    } else if (pid == cnid_dbd_pid) {
        start_val = is_write ? cnid_start_writes : cnid_start_reads;
        end_val = is_write ? cnid_end_writes : cnid_end_reads;
    } else {
        return 0;
    }

    return (end_val > start_val) ? end_val - start_val : 0;
}

/* Safe PID string parsing helper */
static pid_t safe_parse_pid(const char *pid_str)
{
    char *endptr;
    long val;

    if (!pid_str || *pid_str == '\0') {
        return 0;  /* Invalid PID, return 0 to indicate failure */
    }

    /* PIDs should only contain digits */
    if (strspn(pid_str, "0123456789") != strlen(pid_str)) {
        return 0;  /* Invalid PID format */
    }

    errno = 0;
    val = strtol(pid_str, &endptr, 10);

    /* Check for conversion errors and valid PID range */
    /* Use PID_MAX for proper bounds checking instead of INT_MAX */
    if (errno == ERANGE || *endptr != '\0' || val <= 0 || val > PID_MAX) {
        return 0;  /* Invalid PID */
    }

    return (pid_t)val;
}

/* Initialize IO monitoring by checking proc filesystem and finding required processes */
void init_io_monitoring(const char *username)
{
    /* Check if /proc_io is available */
    if (!check_proc_io_availability()) {
        return;
    }

    fprintf(stdout, "IO monitoring: /proc_io is available\n");

    /* Wait for child processes to be created */
    if (Debug) {
        fprintf(stderr,
                "DEBUG: Waiting for login user child processes to be created...\n");
    }

    sleep(1);
    /* Temporarily enable monitoring for process discovery */
    io_monitoring_enabled = 1;
    /* First, search for cnid_dbd (optional - doesn't drop privileges) */
    cnid_dbd_pid = find_process_pid("cnid_dbd", username,
                                    1);  /* Filter -u argument */

    if (cnid_dbd_pid > 0) {
        fprintf(stdout, "Found cnid_dbd process: PID %d\n", cnid_dbd_pid);
    } else {
        fprintf(stdout, "cnid_dbd not found (optional), continuing...\n");
        cnid_dbd_pid = 0;  /* Explicitly set to 0 */
    }

    /* Always search for afpd (mandatory - drops privileges) */
    int32_t attempts = 0;
    const int32_t max_attempts = 3;

    while (attempts < max_attempts) {
        attempts++;
        afpd_pid = find_process_pid("afpd", username, 0);  /* Filter ownership */

        if (afpd_pid > 0) {
            break;
        }

        if (attempts < max_attempts) {
            sleep(1);
        }
    }

    if (afpd_pid <= 0) {
        fprintf(stderr,
                "Warning: Could not find privilege-dropped afpd after %d attempts\n",
                max_attempts);
    }

    if (afpd_pid > 0) {
        fprintf(stdout, "Found privilege-dropped afpd process: PID %d\n", afpd_pid);
    } else {
        fprintf(stderr, "Error: afpd process not found (mandatory)\n");
    }

    /* Only check afpd since it's mandatory; cnid_dbd is optional */
    if (afpd_pid > 0) {
        /* Test that we can actually read from the processes we found */
        int32_t afpd_valid = 0, cnid_dbd_valid = 0;
        uint64_t dummy_read, dummy_write;
        /* Validate afpd (mandatory) */
        afpd_valid = (read_proc_io(afpd_pid, &dummy_read, &dummy_write) == 0);

        if (!afpd_valid) {
            fprintf(stderr, "Error: Cannot read /proc_io/%d/io for afpd\n", afpd_pid);
            afpd_pid = 0;
        }

        /* Validate cnid_dbd if found (optional) */
        if (cnid_dbd_pid > 0) {
            cnid_dbd_valid = (read_proc_io(cnid_dbd_pid, &dummy_read, &dummy_write) == 0);

            if (!cnid_dbd_valid) {
                fprintf(stderr, "Warning: Cannot read /proc_io/%d/io for cnid_dbd (optional)\n",
                        cnid_dbd_pid);
                cnid_dbd_pid = 0;  /* Set to 0 to skip its monitoring */
            }
        }

        /* Keep monitoring enabled only if afpd is valid */
        if (afpd_pid > 0) {
            /* afpd found and /proc_io readable - keep monitoring enabled */
            if (cnid_dbd_pid > 0) {
                fprintf(stdout, "IO monitoring enabled (afpd: %d, cnid_dbd: %d)\n",
                        afpd_pid, cnid_dbd_pid);
            } else {
                fprintf(stdout, "IO monitoring enabled (afpd: %d, cnid_dbd: not found)\n",
                        afpd_pid);
            }
        } else {
            /* /proc_io available but cannot read from afpd - disable monitoring */
            io_monitoring_enabled = 0;
            fprintf(stderr,
                    "IO monitoring disabled: cannot read IO statistics for afpd (mandatory)\n");
        }
    } else {
        /* /proc_io available but afpd not found - disable monitoring */
        io_monitoring_enabled = 0;
        fprintf(stderr, "IO monitoring disabled: afpd not found (mandatory)\n");
    }
}

#else /* !__linux__ */

/* Stub implementations for non-Linux systems */

#include <sys/types.h>
#include <stdint.h>
#include "lantest_io_monitor.h"

/* Global variables - stub versions for non-Linux */
uint8_t io_monitoring_enabled = 0;
pid_t afpd_pid = 0;
pid_t cnid_dbd_pid = 0;
uint64_t afpd_start_reads = 0, afpd_start_writes = 0;
uint64_t cnid_start_reads = 0, cnid_start_writes = 0;
uint64_t afpd_end_reads = 0, afpd_end_writes = 0;
uint64_t cnid_end_reads = 0, cnid_end_writes = 0;

/* Stub function implementations */
int32_t check_proc_io_availability(void)
{
    return 0;  /* IO monitoring not available on non-Linux */
}

pid_t find_process_pid(const char *process_name, const char *username,
                       int32_t filter_by_cmdline)
{
    (void)process_name;
    (void)username;
    (void)filter_by_cmdline;
    return 0;  /* Process not found */
}

void capture_io_values(int32_t is_start)
{
    (void)is_start;
    /* No-op on non-Linux */
}

uint64_t iodiff_io(pid_t pid, int32_t is_write)
{
    (void)pid;
    (void)is_write;
    return 0;  /* No IO data available */
}

void init_io_monitoring(const char *username)
{
    (void)username;
    /* IO monitoring is not available on non-Linux systems */
    io_monitoring_enabled = 0;
}

#endif /* __linux__ */
