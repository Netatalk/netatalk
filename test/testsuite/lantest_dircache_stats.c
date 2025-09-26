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

/* Configuration header (must be first) */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

/* Standard C library includes */
#include <stdio.h>

#ifdef __linux__

/* Standard C library includes for Linux implementation */
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/* Netatalk includes for INIPARSER_GETSTRDUP macro */
#include <atalk/globals.h>

/* Iniparser includes */
#ifdef HAVE_INIPARSER_INIPARSER_H
#include <iniparser/iniparser.h>
#else
#include <iniparser.h>
#endif

/* External globals from lantest.c */
extern bool Debug;

/* Constants */
#define LOG_BUFFER_SIZE (100 * 1024)  /* 100KB buffer for reading log file */
#define MAX_LOG_LINES_DISPLAY 10      /* Number of log lines to show when no stats found */

/* Debug logging macro */
#define DEBUG_LOG(fmt, ...) \
    do { \
        if (Debug) \
            fprintf(stderr, "DEBUG: " fmt "\n", ##__VA_ARGS__); \
    } while(0)

/* Parse netatalk config file to extract log file path */
static char *parse_config_for_log_path(void)
{
    dictionary *iniconfig = NULL;
    char *log_file_path = NULL;
    DEBUG_LOG("Loading config file: %safp.conf", _PATH_CONFDIR);
    /* Load the config file using iniparser */
    iniconfig = iniparser_load(_PATH_CONFDIR "afp.conf");

    if (!iniconfig) {
        fprintf(stderr, "INFO: Could not load config file: %safp.conf\n",
                _PATH_CONFDIR);
        return NULL;
    }

    /* Get the log file value from the Global section */
    log_file_path = INIPARSER_GETSTRDUP(iniconfig, INISEC_GLOBAL, "log file", NULL);

    if (log_file_path && *log_file_path != '\0') {
        DEBUG_LOG("Found log file path: %s", log_file_path);
    } else {
        DEBUG_LOG("No 'log file' configuration found in Global section");

        if (log_file_path) {
            free(log_file_path);
            log_file_path = NULL;
        }
    }

    /* Free the iniparser dictionary */
    iniparser_freedict(iniconfig);
    return log_file_path;
}

/* Read the last portion of log file into buffer
 * Buffer must be at least LOG_BUFFER_SIZE + 1 bytes */
static ssize_t read_log_tail(const char *log_file_path, char *buffer)
{
    FILE *log_fp = NULL;

    /* Validate input parameters */
    if (!buffer) {
        fprintf(stderr, "ERROR: Buffer is NULL\n");
        return -1;
    }

    DEBUG_LOG("Opening log file: %s", log_file_path);
    log_fp = fopen(log_file_path, "rb");

    if (!log_fp) {
        DEBUG_LOG("Failed to open log file: %s (errno=%d: %s)",
                  log_file_path, errno, strerror(errno));
        return -1;
    }

    DEBUG_LOG("Successfully opened log file");

    if (fseek(log_fp, 0, SEEK_END) != 0) {
        fprintf(stderr, "ERROR: Failed to seek to end of log file: %s\n",
                strerror(errno));
        fclose(log_fp);
        return -1;
    }

    long file_size = ftell(log_fp);

    if (file_size == -1L) {
        fprintf(stderr, "ERROR: Failed to determine log file size: %s\n",
                strerror(errno));
        fclose(log_fp);
        return -1;
    }

    DEBUG_LOG("Log file size: %ld bytes", file_size);

    if (file_size <= 0) {
        fprintf(stderr, "WARNING: Log file size is 0, no content to read\n");
        fclose(log_fp);
        return 0;
    }

    /* Read from the end, up to LOG_BUFFER_SIZE to reserve space for null terminator */
    size_t max_read = LOG_BUFFER_SIZE;
    long read_start = (file_size > (long)max_read) ? (file_size -
                      (long)max_read) : 0;
    size_t read_size = (file_size > (long)max_read) ? max_read :
                       (size_t)file_size;
    DEBUG_LOG("Reading %zu bytes from position: %ld", read_size, read_start);

    if (fseek(log_fp, read_start, SEEK_SET) != 0) {
        fprintf(stderr, "ERROR: Failed to seek to read position: %s\n",
                strerror(errno));
        fclose(log_fp);
        return -1;
    }

    size_t bytes_read = fread(buffer, 1, read_size, log_fp);

    if (ferror(log_fp)) {
        fprintf(stderr, "ERROR: Failed to read log file: %s\n", strerror(errno));
        fclose(log_fp);
        return -1;
    }

    /* Simple validation - fread cannot return more than requested */
    if (bytes_read > read_size) {
        fprintf(stderr, "ERROR: fread returned more bytes than requested\n");
        fclose(log_fp);
        return -1;
    }

    /* Null-terminate the buffer
     * Since read_size <= LOG_BUFFER_SIZE and bytes_read <= read_size,
     * we know bytes_read <= LOG_BUFFER_SIZE, so this is always safe */
    buffer[bytes_read] = '\0';
    fclose(log_fp);
    return (ssize_t)bytes_read;
}

/* Search buffer backwards for dircache statistics line */
static char *find_dircache_stats_line(const char *buffer, size_t bytes_read)
{
    char *stats_line = NULL;

    /* Validate bytes_read before using for allocation */
    if (bytes_read == 0 || bytes_read > LOG_BUFFER_SIZE) {
        return NULL;
    }

    /* Make a working copy of the buffer to avoid const issues */
    char *work_buffer = malloc(bytes_read + 1);

    if (!work_buffer) {
        return NULL;
    }

    memcpy(work_buffer, buffer, bytes_read);
    work_buffer[bytes_read] = '\0';
    /* Search backwards through buffer for the LAST dircache statistics line */
    char *current = work_buffer + bytes_read;

    while (current > work_buffer) {
        /* Find end of previous line */
        char *line_end = current - 1;

        while (line_end >= work_buffer && *line_end == '\n') {
            line_end--;
        }

        if (line_end < work_buffer) {
            break;
        }

        /* Find start of this line */
        char *line_start = line_end;

        while (line_start > work_buffer && *(line_start - 1) != '\n') {
            line_start--;
        }

        /* Temporarily null-terminate the line */
        char saved_char = *(line_end + 1);
        *(line_end + 1) = '\0';

        /* Check if this line contains "dircache statistics:" */
        if (strstr(line_start, "dircache statistics:") != NULL) {
            stats_line = strdup(line_start);
            *(line_end + 1) = saved_char;
            break;
        }

        /* Restore the character */
        *(line_end + 1) = saved_char;
        /* Move to previous line */
        current = line_start;
    }

    free(work_buffer);
    return stats_line;
}

/* Display the last N lines from buffer when no stats found */
static void display_last_log_lines(const char *buffer, size_t bytes_read)
{
    /* No dircache statistics found - print message and last 10 lines of log */
    printf("No 'dircache statistics:' logs found.\n\n");
    printf("(At least 'log level = default:info' is required)\n");
    printf("Last 10 lines of log file:\n");
    printf("---------------------------\n");

    /* Validate bytes_read before using for allocation */
    if (bytes_read == 0 || bytes_read > LOG_BUFFER_SIZE) {
        printf("Invalid buffer size\n");
        return;
    }

    /* Make a working copy of the buffer to avoid const issues */
    char *work_buffer = malloc(bytes_read + 1);

    if (!work_buffer) {
        printf("Memory allocation failed\n");
        return;
    }

    memcpy(work_buffer, buffer, bytes_read);
    work_buffer[bytes_read] = '\0';
    /* Search backwards through buffer for last 10 lines */
    char *last_lines[MAX_LOG_LINES_DISPLAY] = {NULL};
    int line_count = 0;
    char *curr = work_buffer + bytes_read;

    while (curr > work_buffer && line_count < MAX_LOG_LINES_DISPLAY) {
        /* Find end of previous line */
        char *l_end = curr - 1;

        while (l_end >= work_buffer && *l_end == '\n') {
            l_end--;
        }

        if (l_end < work_buffer) {
            break;
        }

        /* Find start of this line */
        char *l_start = l_end;

        while (l_start > work_buffer && *(l_start - 1) != '\n') {
            l_start--;
        }

        /* Temporarily null-terminate the line */
        char saved = *(l_end + 1);
        *(l_end + 1) = '\0';
        /* Store this line */
        last_lines[line_count] = strdup(l_start);

        if (last_lines[line_count] != NULL) {
            line_count++;
        }

        /* Restore the character */
        *(l_end + 1) = saved;
        /* Move to previous line */
        curr = l_start;
    }

    free(work_buffer);

    /* Print the last lines in correct order (reverse read order) */
    for (int i = line_count - 1; i >= 0; i--) {
        if (last_lines[i]) {
            /* Get length while pointer is known to be non-NULL */
            size_t len = strlen(last_lines[i]); // NOSONAR: Pointer verified non-NULL
            printf("%s", last_lines[i]);

            /* Add newline if not present */
            if (len > 0 && last_lines[i][len - 1] != '\n') {
                printf("\n");
            }

            free(last_lines[i]);
        }
    }
}

/* Read and display dircache statistics from log file */
void display_dircache_statistics(void)
{
    char *log_file_path = NULL;
    char *stats_line = NULL;
    static char buffer[LOG_BUFFER_SIZE + 1];
    ssize_t bytes_read;
    log_file_path = parse_config_for_log_path();

    if (!log_file_path) {
        fprintf(stderr,
                "INFO: No log file path found in config, skipping dircache statistics\n");
        return;
    }

    DEBUG_LOG("Log file path: %s", log_file_path);
    bytes_read = read_log_tail(log_file_path, buffer, sizeof(buffer));

    if (bytes_read <= 0) {
        free(log_file_path);
        return;
    }

    /* Validate bytes_read is positive and reasonable before casting */
    if (bytes_read < 0 || bytes_read > LOG_BUFFER_SIZE) {
        fprintf(stderr, "ERROR: Invalid bytes_read value: %zd\n", bytes_read);
        free(log_file_path);
        return;
    }

    stats_line = find_dircache_stats_line(buffer, (size_t)bytes_read);
    printf("\nDircache Statistics (%s):\n", log_file_path);
    printf("------------------------------------------------------------------\n");

    if (stats_line) {
        /* Get length while pointer is known to be non-NULL */
        size_t len = strlen(stats_line); // NOSONAR: Pointer verified non-NULL
        printf("%s", stats_line);

        if (len > 0 && stats_line[len - 1] != '\n') {
            printf("\n");
        }

        free(stats_line);
    } else {
        display_last_log_lines(buffer, (size_t)bytes_read);
    }

    free(log_file_path);
}

#else /* !__linux__ */

/* Stub implementation for non-Linux systems */
void display_dircache_statistics(void)
{
    /* Not available on this platform */
}

#endif /* __linux__ */