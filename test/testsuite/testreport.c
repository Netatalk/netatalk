/*
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

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "testreport.h"

struct testreport_case {
    char *name;
    char *message;
    enum testreport_result result;
    double duration;
};

static char *report_path;
static char *report_suite;
static struct testreport_case *report_cases;
static size_t report_case_count;
static size_t report_case_capacity;
static bool report_failed;
static struct timespec report_started;
static struct timespec case_started;

static char *xstrdup(const char *s)
{
    char *copy;

    if (!s) {
        s = "";
    }

    copy = strdup(s);
    return copy;
}

static double elapsed(const struct timespec *start, const struct timespec *end)
{
    return (double)(end->tv_sec - start->tv_sec) +
           (double)(end->tv_nsec - start->tv_nsec) / 1000000000.0;
}

static void now(struct timespec *ts)
{
    clock_gettime(CLOCK_MONOTONIC, ts);
}

static void clear_cases(void)
{
    for (size_t i = 0; i < report_case_count; i++) {
        free(report_cases[i].name);
        free(report_cases[i].message);
    }

    free(report_cases);
    report_cases = NULL;
    report_case_count = 0;
    report_case_capacity = 0;
}

int testreport_configure(const char *path, const char *suite_name)
{
    free(report_path);
    free(report_suite);
    report_path = NULL;
    report_suite = NULL;
    clear_cases();
    report_failed = false;

    if (!path) {
        return 0;
    }

    if (!*path || !strcmp(path, "-")) {
        errno = EINVAL;
        return -1;
    }

    report_path = xstrdup(path);
    report_suite = xstrdup(suite_name);

    if (!report_path || !report_suite) {
        free(report_path);
        free(report_suite);
        report_path = NULL;
        report_suite = NULL;
        errno = ENOMEM;
        return -1;
    }

    now(&report_started);
    return 0;
}

void testreport_begin_case(void)
{
    if (report_path) {
        now(&case_started);
    }
}

static void add_case(const char *name, enum testreport_result result,
                     double duration, const char *message)
{
    struct testreport_case *cases;
    struct testreport_case *item;

    if (!report_path) {
        return;
    }

    if (report_case_count == report_case_capacity) {
        size_t capacity = report_case_capacity ? report_case_capacity * 2 : 64;
        cases = realloc(report_cases, capacity * sizeof(*report_cases));

        if (!cases) {
            report_failed = true;
            return;
        }

        report_cases = cases;
        report_case_capacity = capacity;
    }

    item = &report_cases[report_case_count++];
    item->name = xstrdup(name);
    item->message = xstrdup(message);
    item->result = result;
    item->duration = duration;

    if (!item->name || !item->message) {
        free(item->name);
        free(item->message);
        report_case_count--;
        report_failed = true;
    }
}

void testreport_end_case(const char *name, enum testreport_result result,
                         const char *failure_location,
                         const char *skip_reason)
{
    struct timespec finished;
    const char *message = "";

    if (!report_path) {
        return;
    }

    if (result == TESTREPORT_FAILED) {
        message = failure_location;
    } else if (result == TESTREPORT_SKIPPED) {
        message = skip_reason;
    } else if (result == TESTREPORT_NOT_TESTED) {
        message = "test setup failed";
    }

    now(&finished);
    add_case(name, result, elapsed(&case_started, &finished), message);
}

void testreport_record_case(const char *name, enum testreport_result result,
                            const char *message)
{
    add_case(name, result, 0.0, message);
}

static void xml_escape(FILE *stream, const char *text)
{
    const unsigned char *p;

    if (!text) {
        return;
    }

    p = (const unsigned char *)text;

    for (; *p; p++) {
        switch (*p) {
        case '&':
            fputs("&amp;", stream);
            break;

        case '<':
            fputs("&lt;", stream);
            break;

        case '>':
            fputs("&gt;", stream);
            break;

        case '\"':
            fputs("&quot;", stream);
            break;

        case '\'':
            fputs("&apos;", stream);
            break;

        default:
            if (*p >= 0x20 || *p == '\n' || *p == '\r' || *p == '\t') {
                fputc(*p, stream);
            }
        }
    }
}

static int write_report(FILE *stream)
{
    size_t i;
    unsigned int failures = 0, errors = 0, skipped = 0;
    struct timespec finished;

    for (i = 0; i < report_case_count; i++) {
        if (report_cases[i].result == TESTREPORT_FAILED) {
            failures++;
        } else if (report_cases[i].result == TESTREPORT_NOT_TESTED) {
            errors++;
        } else if (report_cases[i].result == TESTREPORT_SKIPPED) {
            skipped++;
        }
    }

    now(&finished);
    fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<testsuite name=\"", stream);
    xml_escape(stream, report_suite);
    fprintf(stream,
            "\" tests=\"%zu\" failures=\"%u\" errors=\"%u\" skipped=\"%u\" time=\"%.6f\">\n",
            report_case_count, failures, errors, skipped,
            elapsed(&report_started, &finished));

    for (i = 0; i < report_case_count; i++) {
        const struct testreport_case *item = &report_cases[i];
        fputs("  <testcase classname=\"", stream);
        xml_escape(stream, report_suite);
        fputs("\" name=\"", stream);
        xml_escape(stream, item->name);
        fprintf(stream, "\" time=\"%.6f\">", item->duration);

        if (item->result == TESTREPORT_FAILED) {
            fputs("<failure type=\"assertion\" message=\"", stream);
            xml_escape(stream, item->message);
            fputs("\"/>", stream);
        } else if (item->result == TESTREPORT_NOT_TESTED) {
            fputs("<error type=\"setup\" message=\"", stream);
            xml_escape(stream, item->message);
            fputs("\"/>", stream);
        } else if (item->result == TESTREPORT_SKIPPED) {
            fputs("<skipped message=\"", stream);
            xml_escape(stream, item->message);
            fputs("\"/>", stream);
        }

        fputs("</testcase>\n", stream);
    }

    return fputs("</testsuite>\n", stream) == EOF ? -1 : 0;
}

int testreport_finalize(void)
{
    char *temporary;
    size_t temporary_size;
    int fd;
    FILE *stream;
    int result = 0;

    if (!report_path) {
        return 0;
    }

    if (report_failed) {
        errno = ENOMEM;
        clear_cases();
        free(report_path);
        free(report_suite);
        report_path = NULL;
        report_suite = NULL;
        return -1;
    }

    temporary_size = strlen(report_path) + sizeof(".tmp.XXXXXX");
    temporary = malloc(temporary_size);

    if (!temporary) {
        return -1;
    }

    snprintf(temporary, temporary_size, "%s.tmp.XXXXXX", report_path);
    fd = mkstemp(temporary);

    if (fd < 0) {
        free(temporary);
        return -1;
    }

    stream = fdopen(fd, "w");

    if (!stream) {
        close(fd);
        unlink(temporary);
        result = -1;
    } else {
        if (write_report(stream) || fflush(stream) || fsync(fd)) {
            result = -1;
        }

        if (fclose(stream)) {
            result = -1;
        }

        if (result) {
            unlink(temporary);
        } else if (rename(temporary, report_path)) {
            unlink(temporary);
            result = -1;
        }
    }

    free(temporary);
    clear_cases();
    free(report_path);
    free(report_suite);
    report_path = NULL;
    report_suite = NULL;
    return result;
}
