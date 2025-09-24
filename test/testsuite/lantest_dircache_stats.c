/* Standard C library includes */
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/* External globals from lantest.c */
extern bool Debug;

/* Constants */
#define LOG_BUFFER_SIZE (100 * 1024)  /* 100KB buffer for reading log file */
#define MAX_LOG_LINES_DISPLAY 10      /* Number of log lines to show when no stats found */
#define CONFIG_LINE_SIZE 1024         /* Max line size for config file parsing */

/* Debug logging macro */
#define DEBUG_LOG(fmt, ...) \
    do { \
        if (Debug) \
            fprintf(stderr, "DEBUG: " fmt "\n", ##__VA_ARGS__); \
    } while(0)

#ifdef __linux__

/* Parse netatalk config file to extract log file path */
static char *parse_config_for_log_path(void)
{
    FILE *config_fp = NULL;
    char *log_file_path = NULL;
    const char *config_paths[] = {
        "/etc/afp.conf",
        "/etc/netatalk/afp.conf",
        "/usr/local/etc/afp.conf",
        "/usr/local/etc/netatalk/afp.conf",
        NULL
    };

    for (int i = 0; config_paths[i] != NULL; i++) {
        DEBUG_LOG("Trying to open config file: %s", config_paths[i]);

        if (access(config_paths[i], F_OK) != 0) {
            DEBUG_LOG("Config file does not exist: %s", config_paths[i]);
            continue;
        }

        if (access(config_paths[i], R_OK) != 0) {
            DEBUG_LOG("Config file exists but not readable: %s", config_paths[i]);
            continue;
        }

        config_fp = fopen(config_paths[i], "r");

        if (config_fp) {
            DEBUG_LOG("Successfully opened config file: %s", config_paths[i]);
            char line[CONFIG_LINE_SIZE];

            while (fgets(line, sizeof(line), config_fp)) {
                /* Look for "log file = " directive */
                char *ptr = strstr(line, "log file");

                if (ptr) {
                    ptr = strchr(ptr, '=');

                    if (ptr) {
                        ptr++;

                        /* Extract path */
                        while (*ptr && (*ptr == ' ' || *ptr == '\t')) {
                            ptr++;
                        }

                        char *end = ptr;

                        while (*end && *end != '\n' && *end != '\r' && *end != '#') {
                            end++;
                        }

                        if (end > ptr) {
                            *end = '\0';
                            end--;

                            while (end > ptr && (*end == ' ' || *end == '\t')) {
                                *end = '\0';
                                end--;
                            }

                            if (ptr && *ptr != '\0') {
                                DEBUG_LOG("Found log file path: %s", ptr);
                                log_file_path = strdup(ptr);

                                if (!log_file_path) {
                                    fprintf(stderr, "ERROR: strdup() failed for log_file_path\n");
                                    fclose(config_fp);
                                    return NULL;
                                }

                                break;
                            } else {
                                fprintf(stderr, "WARNING: Log file path was empty after trimming\n");
                            }
                        } else {
                            fprintf(stderr, "WARNING: No content after '=' in log file line\n");
                        }
                    } else {
                        fprintf(stderr, "WARNING: No '=' found in log file line\n");
                    }
                }
            }

            fclose(config_fp);

            if (log_file_path) {
                break;
            }
        } else {
            fprintf(stderr, "ERROR: Failed to open config file: %s (errno=%d: %s)\n",
                    config_paths[i], errno, strerror(errno));
        }
    }

    return log_file_path;
}

/* Read the last portion of log file into buffer */
static ssize_t read_log_tail(const char *log_file_path, char *buffer,
                             size_t buffer_size)
{
    FILE *log_fp = NULL;
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

    /* Read from the end, up to buffer_size */
    long read_start = (file_size > (long)buffer_size) ? (file_size -
                      (long)buffer_size) : 0;
    size_t read_size = (file_size > (long)buffer_size) ? buffer_size :
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

    buffer[bytes_read] = '\0';
    fclose(log_fp);
    return (ssize_t)bytes_read;
}

/* Search buffer backwards for dircache statistics line */
static char *find_dircache_stats_line(const char *buffer, size_t bytes_read)
{
    char *stats_line = NULL;
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

#endif /* __linux__ */

/* Read and display dircache statistics from log file */
static void display_dircache_statistics(void)
{
#ifdef __linux__
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
    bytes_read = read_log_tail(log_file_path, buffer, LOG_BUFFER_SIZE);

    if (bytes_read <= 0) {
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
#else
    /* Not on Linux, function not available */
#endif /* __linux__ */
}