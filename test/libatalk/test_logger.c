/*
 * Tests for logger filtering, configuration inheritance and destinations
 *
 * Copyright (c) 2009 Frank Lahm <franklahm@gmail.com>
 * Copyright (c) 2023, 2026 Daniel Markstedt <daniel@mindani.net>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <atalk/logger.h>

static void logger_test_openlog(const char *ident, int options, int facility);
static void logger_test_syslog(int priority, const char *format, ...)
__attribute__((format(printf, 2, 3)));
static void logger_test_closelog(void);

/*
 * Parse syslog.h before renaming libc calls, so fortified declarations stay
 * intact. Compile the implementation here to capture syslog portably, without
 * depending on shared-library symbol interposition. File writes remain real.
 */
#define openlog logger_test_openlog
#define syslog(...) logger_test_syslog(__VA_ARGS__)
#define closelog logger_test_closelog
#include "../../libatalk/util/logger.c"
#undef openlog
#undef syslog
#undef closelog

static int tests_run;
static int tests_failed;
static int open_calls;
static int close_calls;
static char opened_ident[32];
static int opened_options;
static int opened_facility;
static struct {
    int priority;
    char message[256];
} captured[32];
static size_t captured_count;
static int capture_failed;

#ifdef NO_DEBUG
static const int debug_messages = 0;
#else
static const int debug_messages = 1;
#endif

/*!
 * @brief Capture an openlog() call and its connection settings
 *
 * @param[in] ident     process name supplied to syslog
 * @param[in] options   syslog display options
 * @param[in] facility  syslog facility
 */
static void logger_test_openlog(const char *ident, int options, int facility)
{
    open_calls++;
    snprintf(opened_ident, sizeof(opened_ident), "%s", ident);
    opened_options = options;
    opened_facility = facility;
}

/*!
 * @brief Capture a syslog() call's priority and formatted payload
 *
 * Marks the capture as failed if the record array fills, formatting fails,
 * or the payload is truncated, so message counts cannot pass silently.
 *
 * @param[in] priority  syslog message priority
 * @param[in] format    printf-style format for the remaining arguments
 */
static void logger_test_syslog(int priority, const char *format, ...)
{
    va_list args;
    int n;

    if (captured_count == sizeof(captured) / sizeof(captured[0])) {
        capture_failed = 1;
        return;
    }

    captured[captured_count].priority = priority;
    va_start(args, format);
    n = vsnprintf(captured[captured_count].message,
                  sizeof(captured[captured_count].message), format, args);
    va_end(args);

    if (n < 0 || (size_t)n >= sizeof(captured[captured_count].message)) {
        capture_failed = 1;
    }

    captured_count++;
}

/*! @brief Count closelog() calls without touching the host's syslog connection */
static void logger_test_closelog(void)
{
    close_calls++;
}

/*!
 * @brief Print one TAP result line and count failed assertions
 *
 * @param[in] passed  nonzero when the assertion passed
 * @param[in] name    description of the assertion
 */
static void report(int passed, const char *name)
{
    tests_run++;
    printf("%s %d - %s\n", passed ? "ok" : "not ok", tests_run, name);

    if (!passed) {
        tests_failed++;
    }
}

/*!
 * @brief Count captured syslog messages with an exact payload match
 *
 * @param[in] priority  required priority, or a negative value to match any
 * @param[in] message   payload to match, without a log prefix
 *
 * @returns the number of matches, or -1 if capturing failed
 */
static int syslog_message_count(int priority, const char *message)
{
    int count = 0;

    if (capture_failed) {
        return -1;
    }

    for (size_t i = 0; i < captured_count; i++) {
        if ((priority < 0 || captured[i].priority == priority)
                && strcmp(captured[i].message, message) == 0) {
            count++;
        }
    }

    return count;
}

/*!
 * @brief Count file log messages with an exact payload match
 *
 * Ignores variable prefixes and unrelated setup messages.
 *
 * @param[in] path     log file to read
 * @param[in] message  payload to match, without a log prefix or trailing newline
 *
 * @returns the number of matches, or -1 if opening, reading or closing fails
 */
static int file_message_count(const char *path, const char *message)
{
    FILE *file = fopen(path, "r");
    char *line = NULL;
    size_t size = 0;
    int count = 0;

    if (file == NULL) {
        return -1;
    }

    while (getline(&line, &size, file) != -1) {
        const char *payload = strstr(line, "): ");

        if (payload != NULL) {
            payload += 3;
            char *newline = strchr(payload, '\n');

            if (newline != NULL) {
                *newline = '\0';
            }

            if (strcmp(payload, message) == 0) {
                count++;
            }
        }
    }

    if (ferror(file)) {
        count = -1;
    }

    free(line);

    if (fclose(file) != 0) {
        count = -1;
    }

    return count;
}

/*!
 * @brief Check syslog filtering, priorities, inheritance and repeated messages
 *
 * Uses captured calls to verify connection settings and that changing the
 * default threshold preserves an explicitly configured type's level.
 */
static void test_syslog(void)
{
    set_processname("logger_test");
    setuplog("default:severe", NULL, false);
    LOG(log_severe, logtype_logger, "syslog severe");
    report(syslog_message_count(LOG_ERR, "syslog severe") == 1,
           "severe messages reach syslog as LOG_ERR");
    report(open_calls == 1 && strcmp(opened_ident, "logger_test") == 0
           && opened_options == (logoption_ndelay | logoption_pid)
           && opened_facility == logfacility_daemon,
           "syslog opens with the configured process name, options and facility");
    LOG(log_error, logtype_default, "syslog filtered error");
    report(captured_count == 1 && !capture_failed,
           "the severe threshold filters less severe messages");
    setuplog("default:info", NULL, false);
    LOG(log_info, logtype_logger, "syslog formatted %s %d", "value", 7);
    LOG(log_error, logtype_logger, "syslog logger error");
    LOG(log_error, logtype_default, "syslog default error");
    LOG(log_debug, logtype_logger, "syslog filtered logger debug");
    LOG(log_debug, logtype_default, "syslog filtered default debug");
    report(syslog_message_count(LOG_INFO, "syslog formatted value 7") == 1,
           "the inherited info level logs formatted messages as LOG_INFO");
    report(syslog_message_count(LOG_ERR, "syslog logger error") == 1
           && syslog_message_count(LOG_ERR, "syslog default error") == 1,
           "both inherited and default errors reach syslog as LOG_ERR");
    report(syslog_message_count(-1, "syslog filtered logger debug") == 0
           && syslog_message_count(-1, "syslog filtered default debug") == 0,
           "the info threshold filters debug messages for both types");
    setuplog("dsi:maxdebug", NULL, false);
    setuplog("default:error", NULL, false);
    LOG(log_info, logtype_dsi, "syslog explicit info");
    LOG(log_maxdebug, logtype_dsi, "syslog explicit maxdebug");
    LOG(log_info, logtype_afpd, "syslog filtered inherited info");
    LOG(log_error, logtype_afpd, "syslog inherited error");
    report(syslog_message_count(LOG_INFO, "syslog explicit info") == 1
           && type_configs[logtype_dsi].level == log_maxdebug,
           "changing the default preserves an explicit type's syslog level");
    report(syslog_message_count(LOG_DEBUG, "syslog explicit maxdebug")
           == debug_messages,
           "maxdebug maps to LOG_DEBUG when debug logging is compiled in");
    report(syslog_message_count(-1, "syslog filtered inherited info") == 0
           && syslog_message_count(LOG_ERR, "syslog inherited error") == 1,
           "types without an override inherit the changed default threshold");
    report(open_calls == 1 && !capture_failed,
           "syslog configuration changes reuse the open connection");

    for (int i = 0; i < 3; i++) {
        LOG(log_error, logtype_default, "syslog repeated");
    }

    report(syslog_message_count(LOG_ERR, "syslog repeated") == 3,
           "repeated syslog messages are sent once per call");
}

/*!
 * @brief Check file filtering, routing, inheritance and message payloads
 *
 * Changes the default configuration while DSI keeps its own level and file,
 * then checks both destinations and that no messages went to syslog.
 *
 * @param[in] path      default log file in the scratch directory
 * @param[in] dsi_path  separate DSI log file in the scratch directory
 */
static void test_files(const char *path, const char *dsi_path)
{
    captured_count = 0;
    setuplog("default:severe", path, true);
    report(type_configs[logtype_default].fd >= 0,
           "the default log file opens successfully");
    LOG(log_severe, logtype_logger, "file severe");
    LOG(log_error, logtype_default, "file filtered error");
    report(file_message_count(path, "file severe") == 1
           && file_message_count(path, "file filtered error") == 0,
           "file logging inherits the default severe threshold");
    setuplog("dsi:maxdebug", dsi_path, true);
    report(type_configs[logtype_dsi].fd >= 0,
           "the explicit DSI log file opens successfully");
    LOG(log_info, logtype_dsi, "file DSI info");
    LOG(log_info, logtype_afpd, "file filtered inherited info");
    report(file_message_count(dsi_path, "file DSI info") == 1
           && file_message_count(path, "file DSI info") == 0
           && file_message_count(path, "file filtered inherited info") == 0,
           "an explicit file level and destination override the default");
    setuplog("default:debug", path, true);
    LOG(log_debug, logtype_default, "file default debug");
    LOG(log_maxdebug, logtype_default, "file filtered maxdebug");
    LOG(log_maxdebug, logtype_dsi, "file DSI maxdebug");
    LOG(log_info, logtype_afpd, "file inherited info");
    LOG(log_info, logtype_dsi, "file DSI after default change");
    report(file_message_count(path, "file default debug") == debug_messages
           && file_message_count(path, "file filtered maxdebug") == 0,
           "the changed default file level filters messages at its threshold");
    report(file_message_count(path, "file inherited info") == 1
           && file_message_count(dsi_path, "file inherited info") == 0,
           "inherited types follow the default file configuration");
    report(file_message_count(dsi_path, "file DSI after default change") == 1
           && file_message_count(path, "file DSI after default change") == 0
           && file_message_count(dsi_path, "file DSI maxdebug") == debug_messages
           && type_configs[logtype_dsi].level == log_maxdebug,
           "default changes preserve an explicit file level and destination");
    LOG(log_error, logtype_default, "file formatted %s %d", "value", 7);
    report(file_message_count(path, "file formatted value 7") == 1,
           "file logging preserves formatted message payloads");

    /* Repeated messages are preserved; the old flooding suppression is gone. */
    for (int i = 0; i < 3; i++) {
        LOG(log_error, logtype_default, "file repeated");
    }

    report(file_message_count(path, "file repeated") == 3,
           "repeated messages are written once per call");
    report(captured_count == 0 && !capture_failed,
           "file destinations send no messages to syslog");
}

/*!
 * @brief Run the logger tests, clean up their resources and emit the TAP plan
 *
 * @returns 0 when every assertion passes, or 1 on failure or scratch setup error
 */
int main(void)
{
    /* Meson runs here in the build tree; unique names permit parallel runs. */
    char dir[] = "logger_XXXXXX";
    char path[PATH_MAX];
    char dsi_path[PATH_MAX];
    int removed_default;
    int removed_dsi;
    int removed_dir;
    printf("TAP version 13\n");

    if (mkdtemp(dir) == NULL) {
        printf("Bail out! cannot create a scratch directory\n");
        return 1;
    }

    snprintf(path, sizeof(path), "%s/default.log", dir);
    snprintf(dsi_path, sizeof(dsi_path), "%s/dsi.log", dir);
    test_syslog();
    test_files(path, dsi_path);
    log_close_all();
    report(close_calls == 1 && !log_config.syslog_opened
           && type_configs[logtype_default].fd == -1
           && type_configs[logtype_dsi].fd == -1,
           "cleanup closes the syslog connection and both log files");

    for (int i = 0; i < logtype_end_of_list_marker; i++) {
        free(type_configs[i].filename);
        type_configs[i].filename = NULL;
    }

    removed_default = unlink(path) == 0;
    removed_dsi = unlink(dsi_path) == 0;
    removed_dir = rmdir(dir) == 0;
    report(removed_default && removed_dsi && removed_dir,
           "cleanup removes the temporary log files and directory");
    printf("1..%d\n", tests_run);
    return tests_failed == 0 ? 0 : 1;
}
