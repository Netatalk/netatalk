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
#include <atalk/iniparser_util.h>

/* Iniparser includes */
#ifdef HAVE_INIPARSER_INIPARSER_H
#include <iniparser/iniparser.h>
#else
#include <iniparser.h>
#endif

/* External globals from lantest.c */
extern bool Debug;

/* Constants */
/*! 100KB buffer for reading log file */
#define LOG_BUFFER_SIZE (100 * 1024)
/*! Number of log lines to show when no stats found */
#define MAX_LOG_LINES_DISPLAY 10

/*! Static buffer for reading log file - shared across functions */
static char log_buffer[LOG_BUFFER_SIZE + 1];
/*! Actual number of bytes read into log_buffer */
static size_t log_buffer_bytes_read = 0;

/*! Compile-time assertion to ensure buffer size is reasonable */
_Static_assert(LOG_BUFFER_SIZE > 0 && LOG_BUFFER_SIZE <= (1024 * 1024),
               "LOG_BUFFER_SIZE must be between 1 byte and 1MB");

/*! Debug logging macro */
#define DEBUG_LOG(fmt, ...) \
    do { \
        if (Debug) \
            fprintf(stderr, "DEBUG: " fmt "\n", ##__VA_ARGS__); \
    } while(0)

/*! Parse netatalk config file to extract log file path */
static char *parse_config_for_log_path(void)
{
    dictionary *iniconfig = NULL;
    char *log_file_path = NULL;
    DEBUG_LOG("Loading config file: %safp.conf", _PATH_CONFDIR);
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

/*! @brief Read the last portion of log file into the static log_buffer
 * @returns true on success, false on error */
static bool read_log_tail(const char *log_file_path)
{
    FILE *log_fp = NULL;
    /* Reset global state */
    log_buffer_bytes_read = 0;
    DEBUG_LOG("Opening log file: %s", log_file_path);
    log_fp = fopen(log_file_path, "rb");

    if (!log_fp) {
        DEBUG_LOG("Failed to open log file: %s (errno=%d: %s)",
                  log_file_path, errno, strerror(errno));
        return false;
    }

    DEBUG_LOG("Successfully opened log file");

    if (fseek(log_fp, 0, SEEK_END) != 0) {
        fprintf(stderr, "ERROR: Failed to seek to end of log file: %s\n",
                strerror(errno));
        fclose(log_fp);
        return false;
    }

    long file_size = ftell(log_fp);

    if (file_size == -1L) {
        fprintf(stderr, "ERROR: Failed to determine log file size: %s\n",
                strerror(errno));
        fclose(log_fp);
        return false;
    }

    DEBUG_LOG("Log file size: %ld bytes", file_size);

    if (file_size <= 0) {
        fprintf(stderr, "WARNING: Log file size is 0, no content to read\n");
        fclose(log_fp);
        /* Not an error, just no content */
        return true;
    }

    /* Read from the end, up to LOG_BUFFER_SIZE to reserve space for null terminator */
    long read_start = (file_size > LOG_BUFFER_SIZE) ? (file_size - LOG_BUFFER_SIZE)
                      : 0;
    size_t read_size = (file_size > LOG_BUFFER_SIZE) ? LOG_BUFFER_SIZE :
                       (size_t)file_size;
    DEBUG_LOG("Reading %zu bytes from position: %ld", read_size, read_start);

    if (fseek(log_fp, read_start, SEEK_SET) != 0) {
        fprintf(stderr, "ERROR: Failed to seek to read position: %s\n",
                strerror(errno));
        fclose(log_fp);
        return false;
    }

    size_t bytes_read = fread(log_buffer, 1, read_size, log_fp);

    if (ferror(log_fp)) {
        fprintf(stderr, "ERROR: Failed to read log file: %s\n", strerror(errno));
        fclose(log_fp);
        return false;
    }

    /* Explicit bounds check and enforcement for static analyzer */
    if (bytes_read > LOG_BUFFER_SIZE) {
        fprintf(stderr, "ERROR: Read more bytes than buffer size (corruption?)\n");
        fclose(log_fp);
        return false;
    }

    /* Explicitly cap the value to help static analyzer understand the bound */
    if (bytes_read > LOG_BUFFER_SIZE) {
        bytes_read = LOG_BUFFER_SIZE;
    }

    /* Null-terminate the buffer - always safe with static buffer */
    log_buffer[bytes_read] = '\0';
    log_buffer_bytes_read = bytes_read;
    fclose(log_fp);
    return true;
}

/*! @brief Search buffer backwards for a specific dircache statistics line
 * @param pattern  Substring to match, e.g. "dircache statistics (AD|ARC|LRU)"
 * @note Uses the global log_buffer and log_buffer_bytes_read */
static char *find_dircache_stats_line(const char *pattern)
{
    char *stats_line = NULL;

    if (log_buffer_bytes_read == 0) {
        return NULL;
    }

    /* Make a working copy of the static buffer to avoid modifying it
     * log_buffer_bytes_read is guaranteed <= LOG_BUFFER_SIZE by read_log_tail() */
    char *work_buffer = malloc(log_buffer_bytes_read + 1);

    if (!work_buffer) {
        return NULL;
    }

    /* Safe copy - log_buffer_bytes_read is bounded by LOG_BUFFER_SIZE */
    memcpy(work_buffer, log_buffer, log_buffer_bytes_read);
    work_buffer[log_buffer_bytes_read] = '\0';
    /* Search backwards through buffer for the LAST matching line */
    char *current = work_buffer + log_buffer_bytes_read;

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

        /* Check if this line contains the specific pattern */
        if (strstr(line_start, pattern) != NULL) {
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

/*! Display the last N lines from buffer when no stats found */
static void display_last_log_lines(void)
{
    printf("No 'dircache statistics' logs found.\n\n");
    printf("(At least 'log level = default:info' is required)\n");
    printf("Last 10 lines of log file:\n");
    printf("---------------------------\n");

    if (log_buffer_bytes_read == 0) {
        printf("No log content available\n");
        return;
    }

    /* Make a working copy of the static buffer
     * log_buffer_bytes_read is guaranteed <= LOG_BUFFER_SIZE by read_log_tail() */
    char *work_buffer = malloc(log_buffer_bytes_read + 1);

    if (!work_buffer) {
        printf("Memory allocation failed\n");
        return;
    }

    /* Safe copy - log_buffer_bytes_read is bounded by LOG_BUFFER_SIZE */
    memcpy(work_buffer, log_buffer, log_buffer_bytes_read);
    work_buffer[log_buffer_bytes_read] = '\0';
    /* Search backwards through buffer for last 10 lines */
    char *last_lines[MAX_LOG_LINES_DISPLAY] = {NULL};
    int line_count = 0;
    char *curr = work_buffer + log_buffer_bytes_read;

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

/*! Read and display dircache statistics from log file */
void display_dircache_statistics(void)
{
    char *log_file_path = NULL;
    char *stats_line = NULL;
    bool read_success;
    log_file_path = parse_config_for_log_path();

    if (!log_file_path) {
        fprintf(stderr,
                "INFO: No log file path found in config, skipping dircache statistics\n");
        return;
    }

    DEBUG_LOG("Log file path: %s", log_file_path);
    read_success = read_log_tail(log_file_path);

    if (!read_success) {
        free(log_file_path);
        return;
    }

    /* Check if we actually read any content */
    if (log_buffer_bytes_read == 0) {
        printf("\nDircache Statistics (%s):\n", log_file_path);
        printf("------------------------------------------------------------------\n");
        printf("Log file is empty\n");
        free(log_file_path);
        return;
    }

    /* Search for each dircache statistics type separately.
     * The server logs three stat lines: ARC (or LRU), and AD. */
    printf("\nDircache Statistics (%s):\n", log_file_path);
    printf("------------------------------------------------------------------\n");
    /* Try ARC first, fall back to LRU */
    stats_line = find_dircache_stats_line("dircache statistics (ARC)");

    if (!stats_line) {
        stats_line = find_dircache_stats_line("dircache statistics (LRU)");
    }

    char *ad_line = find_dircache_stats_line("dircache statistics (AD)");
    char *hints_line = find_dircache_stats_line("dircache statistics (hints)");
    char *ghost_line = find_dircache_stats_line("ARC ghost performance");
    char *table_line = find_dircache_stats_line("ARC table state");
    char *adapt_line = find_dircache_stats_line("ARC adaptation");
    char *ops_line = find_dircache_stats_line("ARC operations");
    /* Print all found stats lines */
    char *lines[] = { stats_line, ghost_line, table_line, adapt_line,
                      ops_line, hints_line, ad_line
                    };
    int num_lines = sizeof(lines) / sizeof(lines[0]);
    int found_any = 0;

    for (int i = 0; i < num_lines; i++) {
        if (lines[i]) {
            found_any = 1;
        }
    }

    if (found_any) {
        for (int i = 0; i < num_lines; i++) {
            if (lines[i]) {
                size_t len = strnlen(lines[i], LOG_BUFFER_SIZE);
                printf("%s", lines[i]);

                if (len > 0 && lines[i][len - 1] != '\n') {
                    printf("\n");
                }

                free(lines[i]);
            }
        }
    } else {
        display_last_log_lines();
    }

    free(log_file_path);
}

#else /* !__linux__ */

/*! Stub implementation for non-Linux systems */
void display_dircache_statistics(void)
{
    /* Not available on this platform */
}

#endif /* __linux__ */