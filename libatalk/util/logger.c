/* =========================================================================

logger.c was written by Simon Bazley (sibaz@sibaz.com)

I believe libatalk is released under the L/GPL licence.
Just incase, it is, thats the licence I'm applying to this file.
Netatalk 2001 (c)
Copyright (c) 2026 Andy Lemin (andylemin)

 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include <atalk/compat.h>
#include <atalk/logger.h>
#include <atalk/unix.h>
#include <atalk/util.h>

#define COUNT_ARRAY(array) (sizeof((array))/sizeof((array)[0]))

#define MAXLOGSIZE 512

#define LOGLEVEL_STRING_IDENTIFIERS { \
  "-",                      \
  "severe",                       \
  "error",                        \
  "warn",                         \
  "note",                         \
  "info",                         \
  "debug",                        \
  "debug6",                       \
  "debug7",                       \
  "debug8",                       \
  "debug9",                       \
  "maxdebug"}

/*! these are the string identifiers corresponding to each logtype */
#define LOGTYPE_STRING_IDENTIFIERS { \
  "Default",                         \
  "Logger",                          \
  "CNID",                            \
  "AFPDaemon",                       \
  "DSI",                             \
  "ATalkDaemon",                     \
  "PAPDaemon",                       \
  "UAMS",                            \
  "FCE",                             \
  "ad",                              \
  "Spotlight",                       \
  "end_of_list_marker"}

/* =========================================================================
   Config
 */

/*! Main log config container */
log_config_t log_config = { 0 };

/*! Default log config: log nothing to files.
 @code
   0:               set ?
   0:               syslog ?
   -1:              logfiles fd
   log_none:        no logging by default
   0:               Display options
   true             timestamp_us
   NULL             filename (implicit)
 @endcode
 */
#define DEFAULT_LOG_CONFIG {0, 0, -1, log_none, 0, true}

UAM_MODULE_EXPORT logtype_conf_t type_configs[logtype_end_of_list_marker] = {
    DEFAULT_LOG_CONFIG, /* logtype_default */
    DEFAULT_LOG_CONFIG, /* logtype_logger */
    DEFAULT_LOG_CONFIG, /* logtype_cnid */
    DEFAULT_LOG_CONFIG, /* logtype_afpd */
    DEFAULT_LOG_CONFIG, /* logtype_dsi */
    DEFAULT_LOG_CONFIG, /* logtype_atalkd */
    DEFAULT_LOG_CONFIG, /* logtype_papd */
    DEFAULT_LOG_CONFIG, /* logtype_uams */
    DEFAULT_LOG_CONFIG, /* logtype_fce */
    DEFAULT_LOG_CONFIG, /* logtype_ad */
    DEFAULT_LOG_CONFIG  /* logtype_sl */
};


/* These are used by the LOG macro to store __FILE__ and __LINE__ */
static const char *log_src_filename;
static int  log_src_linenumber;

/* Array to store text to list given a log type */
static const char *arr_logtype_strings[] =  LOGTYPE_STRING_IDENTIFIERS;
static const unsigned int num_logtype_strings =
    COUNT_ARRAY(arr_logtype_strings);

/* Array for charachters representing log severity in the log file */
static const char arr_loglevel_chars[] = {'-', 'S', 'E', 'W', 'N', 'I', 'D'};
static const unsigned int num_loglevel_chars = COUNT_ARRAY(arr_loglevel_chars);

static const char *arr_loglevel_strings[] = LOGLEVEL_STRING_IDENTIFIERS;
static const unsigned int num_loglevel_strings =
    COUNT_ARRAY(arr_loglevel_strings);

/* =========================================================================
   Internal function definitions
 */

/*!
 * @brief Format a log file line around a message
 *
 * The line reads "time process[pid] {file:line} (level:type): message" and
 * ends with a newline. The source file and line come from the static
 * position that make_log_entry sets.
 *
 * @param[out] message_details_buffer  receives the line, which the caller
 *                                     frees, or "" on failure
 * @param[in]  user_message            message text
 * @param[in]  display_options         unused
 * @param[in]  loglevel                level of the message
 * @param[in]  logtype                 type of the message
 * @param[in]  log_us_timestamp        add microseconds to the time
 * @returns the line length, or -1 when asprintf fails
 */
static int generate_message(char **message_details_buffer,
                            char *user_message,
                            int display_options _U_,
                            enum loglevels loglevel,
                            enum logtypes logtype,
                            bool log_us_timestamp)
{
    char *details;
    int    len;
    struct timeval tv;
    pid_t  pid;
    char buf[256];
    gettimeofday(&tv, NULL);
    strftime(buf, sizeof(buf), "%b %d %H:%M:%S", localtime(&tv.tv_sec));
    pid = getpid();
    const char *basename = strrchr(log_src_filename, '/');

    if (basename) {
        basename++;
    } else {
        basename = log_src_filename;
    }

    if (log_us_timestamp) {
        len = asprintf(&details,
                       "%s.%06u %s[%d] {%s:%d} (%s:%s): %s\n",
                       buf,
                       (int)tv.tv_usec,
                       log_config.processname,
                       pid,
                       basename,
                       log_src_linenumber,
                       arr_loglevel_strings[loglevel],
                       arr_logtype_strings[logtype],
                       user_message);
    } else {
        len = asprintf(&details,
                       "%s %s[%d] {%s:%d} (%s:%s): %s\n",
                       buf,
                       log_config.processname,
                       pid,
                       basename,
                       log_src_linenumber,
                       arr_loglevel_strings[loglevel],
                       arr_logtype_strings[logtype],
                       user_message);
    }

    if (len == -1) {
        *message_details_buffer = "";
        return -1;
    }

    *message_details_buffer = details;
    return len;
}

/*!
 * @brief Map a netatalk log level to a syslog priority
 *
 * Both log_severe and log_error map to LOG_ERR. Every debug level maps to
 * LOG_DEBUG.
 *
 * @param[in] loglevel  netatalk level
 * @returns the syslog priority
 */
static int get_syslog_equivalent(enum loglevels loglevel)
{
    switch (loglevel) {
    /* The question is we know how bad it is for us,
       but how should that translate in the syslogs?  */
    case 1:
        /* severe */
        return LOG_ERR;

    case 2:
        /* error */
        return LOG_ERR;

    case 3:
        /* warning */
        return LOG_WARNING;

    case 4:
        /* note */
        return LOG_NOTICE;

    case 5:
        /* information */
        return LOG_INFO;

    default:
        /* debug */
        return LOG_DEBUG;
    }
}

/*!
 * @brief Send a message to syslog, opening the connection on first use
 *
 * The make_log_entry call sends the messages of syslog types here.
 *
 * @param[in] loglevel  level of the message
 * @param[in] logtype   unused
 * @param[in] message   message text
 */
static void make_syslog_entry(enum loglevels loglevel,
                              enum logtypes logtype _U_, char *message)
{
    if (!log_config.syslog_opened) {
        openlog(log_config.processname,
                log_config.syslog_display_options,
                log_config.syslog_facility);
        log_config.syslog_opened = true;
    }

    syslog(get_syslog_equivalent(loglevel), "%s", message);
}

/*!
 * @brief Send every log type to syslog at log_info
 *
 * The make_log_entry call runs it when the process logs before any setup.
 */
static void log_init(void)
{
    syslog_setup(log_info,
                 logtype_default,
                 logoption_ndelay | logoption_pid,
                 logfacility_daemon);
}

/*!
 * @brief Open a log file for appending, as root and close-on-exec
 *
 * A new file gets mode 0640, because logs can hold user names and paths.
 *
 * @param[in] filename  path of the log file
 * @returns the descriptor, or -1 with errno set
 */
static int log_open_file(const char *filename)
{
    int fd;
    become_root();
    /* Logs can contain user names and paths; keep them private to owner/group. */
    fd = open(filename, O_CREAT | O_WRONLY | O_APPEND | O_CLOEXEC,
              S_IRUSR | S_IWUSR | S_IRGRP);
    unbecome_root();
    return fd;
}

/* the file each type's descriptor was opened on, to tell it from a stranger */
static dev_t log_file_dev[logtype_end_of_list_marker];
static ino_t log_file_ino[logtype_end_of_list_marker];

/* lines lost since the last write that worked, why the first failed, and the
 * process that lost them */
static unsigned long log_lost[logtype_end_of_list_marker];
static int log_lost_errno[logtype_end_of_list_marker];
static pid_t log_lost_pid[logtype_end_of_list_marker];

/* the file ends in part of a line whose rest could not be written */
static bool log_file_midline[logtype_end_of_list_marker];

/*!
 * @brief Record the device and inode of the file behind the new descriptor of a
 * type
 *
 * They let log_file_reopen tell the file from another one under the same
 * name or number. A failed fstat records 0 for both. The call also clears
 * the mid-line mark of the type.
 *
 * @param[in] logtype  type with the new descriptor
 */
static void log_file_opened(enum logtypes logtype)
{
    struct stat st;
    log_file_dev[logtype] = 0;
    log_file_ino[logtype] = 0;
    log_file_midline[logtype] = false;

    if (fstat(type_configs[logtype].fd, &st) == 0) {
        log_file_dev[logtype] = st.st_dev;
        log_file_ino[logtype] = st.st_ino;
    }
}

/*!
 * @brief Return the absolute name that reopens a log file after a chdir
 *
 * A relative name joins the current directory.
 *
 * @param[in] filename  log file name
 * @returns the new absolute path, or NULL on failure
 */
static char *log_absolute_name(const char *filename)
{
    char cwd[PATH_MAX];
    char *abs;

    if (filename[0] == '/') {
        return strdup(filename);
    }

    if (getcwd(cwd, sizeof(cwd)) == NULL
            || asprintf(&abs, "%s/%s", cwd, filename) == -1) {
        return NULL;
    }

    return abs;
}

/*!
 * @brief Point a log type at a file, or turn the type off with a level of 0
 *
 * A level of 0 closes the file of a set type. For the default type it also
 * turns off every type that has no setup. Any other level drops the old
 * setup of the type and opens the new file. "/dev/tty" means stdout, and a
 * name that ends in XXXXXX goes through mkstemp. Any other name opens as
 * root. A file without an absolute name to reopen it by stays closed, and
 * the type stays unset.
 *
 * The default type passes its level and timestamp setting to every type
 * that has no setup, so the LOG macro reads one level per type.
 *
 * @param[in] filename          log file, "/dev/tty", or a mkstemp template
 * @param[in] loglevel          level, 0 to turn the type off
 * @param[in] logtype           type to set up
 * @param[in] log_us_timestamp  add microseconds to the time, passed on by the
 *                              default type
 */
static void log_setup(const char *filename, enum loglevels loglevel,
                      enum logtypes logtype, const bool log_us_timestamp)
{
    if (loglevel == 0) {
        /* Disable */
        if (type_configs[logtype].set) {
            if (type_configs[logtype].fd != -1) {
                close(type_configs[logtype].fd);
            }

            type_configs[logtype].fd = -1;
            free(type_configs[logtype].filename);
            type_configs[logtype].filename = NULL;
            type_configs[logtype].level = -1;
            type_configs[logtype].set = false;

            /* if disabling default also set all "default using" levels to 0 */
            if (logtype == logtype_default) {
                while (logtype != logtype_end_of_list_marker) {
                    if (!(type_configs[logtype].set)) {
                        type_configs[logtype].level = -1;
                    }

                    logtype++;
                }
            }
        }

        return;
    }

    /* Safety check */
    if (NULL == filename) {
        return;
    }

    /* Resetting existing config ? fd 1 is "/dev/tty", not ours to close */
    if (type_configs[logtype].set) {
        if (type_configs[logtype].fd > 1) {
            close(type_configs[logtype].fd);
        }

        type_configs[logtype].fd = -1;
        free(type_configs[logtype].filename);
        type_configs[logtype].filename = NULL;
        type_configs[logtype].level = -1;
        type_configs[logtype].set = false;
        type_configs[logtype].syslog = false;

        /* Reset configs using default */
        if (logtype == logtype_default) {
            int typeiter = 0;

            while (typeiter != logtype_end_of_list_marker) {
                if (type_configs[typeiter].set == false) {
                    type_configs[typeiter].level = -1;
                    type_configs[typeiter].syslog = false;
                }

                typeiter++;
            }
        }
    }

    /* Set new values */
    type_configs[logtype].level = loglevel;

    /* Open log file as OPEN_LOGS_AS_UID*/

    /* Is it /dev/tty ? */
    if (strcmp(filename, "/dev/tty") == 0) {
        type_configs[logtype].fd = 1; /* stdout */
        /* Does it end in "XXXXXX" ? debug reguest via SIGINT */
    } else if (strlen(filename) >= 6
               && strcmp(filename + strlen(filename) - 6, "XXXXXX") == 0) {
        char *tmp = strdup(filename);
        type_configs[logtype].fd = mkstemp(tmp);

        /* the generated name is what log_reopen() opens again */
        if (type_configs[logtype].fd != -1) {
            type_configs[logtype].filename = log_absolute_name(tmp);
        }

        free(tmp);
    } else {
        type_configs[logtype].fd = log_open_file(filename);

        if (type_configs[logtype].fd != -1) {
            type_configs[logtype].filename = log_absolute_name(filename);
        }
    }

    /* a file that cannot be reopened by name is not set up: after a
     * daemonize() its descriptor would be stale */
    if (type_configs[logtype].fd > 1 && type_configs[logtype].filename == NULL) {
        close(type_configs[logtype].fd);
        type_configs[logtype].fd = -1;
    }

    /* Check for error opening/creating logfile */
    if (type_configs[logtype].fd == -1) {
        type_configs[logtype].level = -1;
        type_configs[logtype].set = false;
        return;
    }

    fcntl(type_configs[logtype].fd, F_SETFD, FD_CLOEXEC);
    type_configs[logtype].set = true;
    type_configs[logtype].syslog = false;
    log_config.inited = true;
    log_file_opened(logtype);
    log_lost[logtype] = 0;

    /* Here's how we make it possible to LOG to a logtype like "logtype_afpd" */
    /* which then uses the default logtype setup if it isn't setup itself: */
    /* we just copy the loglevel from default to all logtypes that are not setup. */
    /* In "make_log_entry" we then check for the logtypes if they arent setup */
    /* and use default then. We must provide accessible values for all logtypes */
    /* in order to make it easy and fast to check the loglevels in the LOG macro! */

    if (logtype == logtype_default) {
        int typeiter = 0;

        while (typeiter != logtype_end_of_list_marker) {
            if (!(type_configs[typeiter].set)) {
                type_configs[typeiter].level = loglevel;
                type_configs[typeiter].timestamp_us = log_us_timestamp;
            }

            typeiter++;
        }
    }

    LOG(log_debug, logtype_logger,
        "Setup file logging: type: %s, level: %s, file: %s, timestamp_us: %d",
        arr_logtype_strings[logtype], arr_loglevel_strings[loglevel], filename,
        log_us_timestamp);
}

/*!
 * @brief Close the syslog connection and every log file before a closeall()
 *
 * Called while the descriptors are still ours: closelog() on a closed and
 * reused number would close the newcomer, and a stale file number kept in
 * type_configs would be closed by log_reopen() after another type had already
 * been given it. The next syslog message opens a new connection; log_reopen()
 * reopens the files.
 */
void log_close_all(void)
{
    if (log_config.syslog_opened) {
        closelog();
        log_config.syslog_opened = false;
    }

    for (int i = 0; i < logtype_end_of_list_marker; i++) {
        if (type_configs[i].set && type_configs[i].filename != NULL
                && type_configs[i].fd != -1) {
            close(type_configs[i].fd);
            type_configs[i].fd = -1;
        }
    }
}

/*!
 * @brief Reopen the log files a previous setuplog() opened
 *
 * daemonize() closes every descriptor, so a process that set up logging
 * before it daemonized would write to a closed or reused one. Files opened by
 * name are reopened by it, a mkstemp() file by the name it got; fd 1 for
 * "/dev/tty" is not ours to close. Only a type whose descriptor
 * log_close_all() has already closed is reopened, so a number another type
 * was just given is never closed here. A file that cannot be reopened falls
 * back to syslog, as a type with no file does.
 */
void log_reopen(void)
{
    for (int i = 0; i < logtype_end_of_list_marker; i++) {
        if (!type_configs[i].set || type_configs[i].filename == NULL
                || type_configs[i].fd != -1) {
            continue;
        }

        type_configs[i].fd = log_open_file(type_configs[i].filename);

        if (type_configs[i].fd == -1) {
            int err = errno;
            syslog_setup(type_configs[i].level, i,
                         logoption_ndelay | logoption_pid, logfacility_daemon);
            LOG(log_error, i, "log_reopen: %s: %s", type_configs[i].filename,
                strerror(err));
        } else {
            log_file_opened(i);
        }
    }
}

/*!
 * @brief Send a log type to syslog at a level
 *
 * The display options and the facility apply to every type. For the default
 * type, every type that has no setup takes the level and syslog. An open log
 * file of the type stays open, unused.
 *
 * @param[in] loglevel         level
 * @param[in] logtype          type to set up
 * @param[in] display_options  openlog options
 * @param[in] facility         syslog facility
 */
void syslog_setup(int loglevel, enum logtypes logtype, int display_options,
                  int facility)
{
    /*
     * FIXME:
     * this currently doesn't care if logtype is already logging to a file.
     * Fortunately currently there's no way a user could trigger this as afpd.conf
     * is not re-read on SIGHUP.
     */
    type_configs[logtype].level = loglevel;
    type_configs[logtype].set = true;
    type_configs[logtype].syslog = true;
    log_config.syslog_display_options = display_options;
    log_config.syslog_facility = facility;

    /* Setting default logging? Then set all logtype not set individually */
    if (logtype == logtype_default) {
        int typeiter = 0;

        while (typeiter != logtype_end_of_list_marker) {
            if (!(type_configs[typeiter].set)) {
                type_configs[typeiter].level = loglevel;
                type_configs[typeiter].syslog = true;
            }

            typeiter++;
        }
    }

    log_config.inited = 1;
    LOG(log_info, logtype_logger, "Set syslog logging to level: %s",
        arr_loglevel_strings[loglevel]);
}

/*!
 * @brief Set up one log type from its type and level names
 *
 * Both names match without regard to case, and an unknown name leaves the
 * setup unchanged. A NULL level turns the type off. A NULL filename sends
 * the type to syslog through syslog_setup, and any other name goes to
 * log_setup.
 *
 * @param[in] loglevel          level name, or NULL
 * @param[in] logtype           type name
 * @param[in] filename          log file, or NULL for syslog
 * @param[in] log_us_timestamp  add microseconds to the time
 */
static void setuplog_internal(const char *loglevel, const char *logtype,
                              const char *filename, const bool log_us_timestamp)
{
    unsigned int typenum, levelnum;

    /* Parse logtype */
    for (typenum = 0; typenum < num_logtype_strings; typenum++) {
        if (strcasecmp(logtype, arr_logtype_strings[typenum]) == 0) {
            break;
        }
    }

    if (typenum >= (unsigned int)logtype_end_of_list_marker) {
        return;
    }

    /* Parse loglevel */
    if (loglevel == NULL) {
        levelnum = 0;
    } else {
        for (levelnum = 1; levelnum < num_loglevel_strings; levelnum++) {
            if (strcasecmp(loglevel, arr_loglevel_strings[levelnum]) == 0) {
                break;
            }
        }

        if (levelnum >= num_loglevel_strings) {
            return;
        }
    }

    /* is this a syslog setup or a filelog setup ? */
    if (filename == NULL) {
        /* must be syslog */
        syslog_setup(levelnum,
                     typenum,
                     logoption_ndelay | logoption_pid,
                     logfacility_daemon);
    } else {
        /* this must be a filelog */
        log_setup(filename, levelnum, typenum, log_us_timestamp);
    }

    return;
}

/*!
 * @brief Reopen a log file by name after a failed write, where that can help
 *
 * A reopen helps when the name no longer names the open file, as after a
 * delete or a rotation, or when the descriptor is no longer the file's; a
 * full disk under the same name gains nothing from one.  The new descriptor
 * takes the slot before the old one is closed, so no slot holds a closed
 * number.
 *
 * @param[in] owner  type whose slot holds the descriptor
 * @param[in] fd     descriptor the write failed on
 * @param[in] err    errno of the failed write
 *
 * @returns true when the slot holds a new descriptor to retry on
 */
static bool log_file_reopen(enum logtypes owner, int fd, int err)
{
    const char *name = type_configs[owner].filename;
    struct stat st;
    bool ours;
    int newfd;

    /* fd 1 for "/dev/tty" has no name to reopen */
    if (name == NULL) {
        return false;
    }

    ours = err != EBADF && fstat(fd, &st) == 0
           && st.st_dev == log_file_dev[owner] && st.st_ino == log_file_ino[owner];

    if (ours && stat(name, &st) == 0
            && st.st_dev == log_file_dev[owner] && st.st_ino == log_file_ino[owner]) {
        return false;
    }

    newfd = log_open_file(name);

    if (newfd == -1) {
        if (!ours) {
            type_configs[owner].fd = -1;
        }

        return false;
    }

    type_configs[owner].fd = newfd;
    log_file_opened(owner);

    if (ours) {
        close(fd);
    }

    return true;
}

/*!
 * @brief Write a formatted line to the log file of a type, retrying once on a
 * reopened file
 *
 * A short write goes on with the rest of the line. After a reopen the new
 * file gets the line from its start. A line that stops partway leaves the
 * file mid-line, and the next line there starts with a newline. EINTR
 * repeats the write.
 *
 * @param[in] owner  type whose slot holds the descriptor
 * @param[in] line   formatted line
 * @param[in] len    length of line
 * @returns 0, or the errno of the failed write, where a write of 0 bytes counts
 *          as EIO
 */
static int log_file_write(enum logtypes owner, const char *line, size_t len)
{
    bool reopened = false;
    size_t done = 0;

    if (log_file_midline[owner]) {
        if (write(type_configs[owner].fd, "\n", 1) != 1) {
            return errno;
        }

        log_file_midline[owner] = false;
    }

    while (done < len) {
        ssize_t n = write(type_configs[owner].fd, line + done, len - done);
        int err;

        if (n > 0) {
            done += (size_t)n;
            continue;
        }

        err = n == 0 ? EIO : errno;

        if (err == EINTR) {
            continue;
        }

        if (reopened || !log_file_reopen(owner, type_configs[owner].fd, err)) {
            log_file_midline[owner] = done > 0;
            return err;
        }

        reopened = true;
        done = 0;
    }

    return 0;
}

/*!
 * @brief Give a type with an empty slot its file back, once the file opens
 * again
 *
 * A failed reopen empties the slot, and make_log_entry calls this for every
 * line until the open works.
 *
 * @param[in] owner  type whose slot holds the descriptor
 * @returns true when the slot holds a descriptor, false after a failed open or
 *          for a type without a file name
 */
static bool log_file_recover(enum logtypes owner)
{
    if (type_configs[owner].fd >= 0) {
        return true;
    }

    if (type_configs[owner].filename == NULL) {
        return false;
    }

    type_configs[owner].fd = log_open_file(type_configs[owner].filename);

    if (type_configs[owner].fd == -1) {
        return false;
    }

    log_file_opened(owner);
    return true;
}

/*!
 * @brief Write the count of unwritten lines and the first error to the log file
 * of a type
 *
 * The make_log_entry call writes this note before the next message. The note
 * carries the source position of this file, and the call restores the
 * position of the caller afterwards. A written note resets the count.
 *
 * @param[in] owner             type whose lines failed to reach its file
 * @param[in] log_us_timestamp  add microseconds to the time
 */
static void log_file_note_lost(enum logtypes owner, bool log_us_timestamp)
{
    const char *caller_file = log_src_filename;
    int caller_line = log_src_linenumber;
    char text[128];
    char *line;
    int len;
    snprintf(text, sizeof(text), "lost %lu log lines: %s", log_lost[owner],
             strerror(log_lost_errno[owner]));
    log_src_filename = __FILE__;
    log_src_linenumber = __LINE__;
    len = generate_message(&line, text, type_configs[owner].display_options,
                           log_error, logtype_logger, log_us_timestamp);
    log_src_filename = caller_file;
    log_src_linenumber = caller_line;

    if (len == -1) {
        return;
    }

    if (log_file_write(owner, line, len) == 0) {
        log_lost[owner] = 0;
    }

    free(line);
}

/* =========================================================================
   Global function definitions
 */

/*!
 * @brief Set the process name of log lines and syslog, cut to 15 characters
 *
 * @param[in] processname  name of the process
 */
void set_processname(const char *processname)
{
    strncpy(log_config.processname, processname, 15);
    log_config.processname[15] = 0;
}

/*!
 * @brief Write a log message to syslog or to the log file of its type, the
 * worker of the LOG macro
 *
 * A call from a signal handler during another call returns at once. The call
 * leaves errno unchanged. A process without any setup logs to syslog at
 * log_info. A type without a setup writes through the file of the default
 * type.
 *
 * A line that cannot reach its file adds to a count of unwritten lines. The
 * next line that reaches the file follows a note with the count and the
 * first error. A forked child drops the count of its parent.
 *
 * @param[in] loglevel          level of the message
 * @param[in] logtype           type of the message
 * @param[in] file              source file of the LOG call
 * @param[in] log_us_timestamp  add microseconds to the time
 * @param[in] line              source line of the LOG call
 * @param[in] message           printf format
 * @param[in] ...               format arguments
 */
void make_log_entry(enum loglevels loglevel, enum logtypes logtype,
                    const char *file, const bool log_us_timestamp, int line, char *message, ...)
{
    /* fn is not reentrant but is used in signal handler
     * with LOGGER it's a little late source name and line number
     * are already changed. */
    static volatile sig_atomic_t inlog = 0;
    enum logtypes owner;
    int len, err;
    char *user_message, *log_message;
    va_list args;
    /* A logger must not perturb the caller's errno: the write()/vasprintf()
     * below set it, and callers commonly log on an error path and then act on
     * errno.  Saved on entry, restored at exit. */
    int saved_errno = errno;

    if (inlog) {
        errno = saved_errno;
        return;
    }

    inlog = 1;

    if (!log_config.inited) {
        log_init();
    }

    if (type_configs[logtype].syslog) {
        if (type_configs[logtype].level >= loglevel) {
            /* Initialise the Messages and send it to syslog */
            va_start(args, message);
            len = vasprintf(&user_message, message, args);
            va_end(args);

            if (len == -1) {
                inlog = 0;
                errno = saved_errno;
                return;
            }

            make_syslog_entry(loglevel, logtype, user_message);
            free(user_message);
        }

        inlog = 0;
        errno = saved_errno;
        return;
    }

    /* logging to a file */
    log_src_filename = file;
    log_src_linenumber = line;
    /* a type that is not set up writes through the default's file */
    owner = type_configs[logtype].set ? logtype : logtype_default;

    /* a forked process inherits its parent's count, which is not its own */
    if (log_lost[owner] != 0 && log_lost_pid[owner] != getpid()) {
        log_lost[owner] = 0;
    }

    /* a slot emptied by a failed reopen is tried again for every line */
    if (!log_file_recover(owner)) {
        if (type_configs[owner].filename != NULL) {
            /* the line is lost; it has no text yet, so it only counts */
            if (log_lost[owner]++ == 0) {
                log_lost_errno[owner] = errno;
                log_lost_pid[owner] = getpid();
            }
        }

        goto exit;
    }

    /* Initialise the Messages */
    va_start(args, message);
    len = vasprintf(&user_message, message, args);
    va_end(args);

    if (len == -1) {
        goto exit;
    }

    len = generate_message(&log_message,
                           user_message,
                           type_configs[logtype].set ?
                           type_configs[logtype].display_options :
                           type_configs[logtype_default].display_options,
                           loglevel, logtype, log_us_timestamp);

    if (len == -1) {
        free(user_message);
        goto exit;
    }

    if (log_lost[owner] != 0) {
        log_file_note_lost(owner, log_us_timestamp);
    }

    /* the note's write can leave the slot empty */
    if (type_configs[owner].fd < 0) {
        err = EBADF;
    } else {
        err = log_file_write(owner, log_message, len);
    }

    if (err != 0 && log_lost[owner]++ == 0) {
        log_lost_errno[owner] = err;
        log_lost_pid[owner] = getpid();
    }

    free(log_message);
    free(user_message);
exit:
    inlog = 0;
    errno = saved_errno;
}

/*!
 * @brief Set up logging from a list of type:level pairs
 *
 * Commas or spaces separate the pairs, as in "default:note afpdaemon:debug".
 * Type and level names match without regard to case, and the call skips a
 * pair with an unknown name.
 *
 * @param[in] logstr            list of type:level pairs
 * @param[in] logfile           log file, "/dev/tty", a mkstemp template, or
 *                              NULL for syslog
 * @param[in] log_us_timestamp  add microseconds to the time
 */
void setuplog(const char *logstr, const char *logfile,
              const bool log_us_timestamp)
{
    char *ptr, *save;
    char *logtype, *loglevel;
    char c;
    save = ptr = strdup(logstr);
    ptr = strtok(ptr, ", ");

    while (ptr) {
        while (*ptr) {
            while (*ptr && isspace(*ptr)) {
                ptr++;
            }

            logtype = ptr;
            ptr = strpbrk(ptr, ":");

            if (!ptr) {
                break;
            }

            *ptr = 0;
            ptr++;
            loglevel = ptr;

            while (*ptr && !isspace(*ptr)) {
                ptr++;
            }

            c = *ptr;
            *ptr = 0;
            setuplog_internal(loglevel, logtype, logfile, log_us_timestamp);
            *ptr = c;
        }

        ptr = strtok(NULL, ", ");
    }

    free(save);
}
