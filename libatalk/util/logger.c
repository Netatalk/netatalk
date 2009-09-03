#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* =========================================================================

logger.c was written by Simon Bazley (sibaz@sibaz.com)

I believe libatalk is released under the L/GPL licence.
Just incase, it is, thats the licence I'm applying to this file.
Netatalk 2001 (c)

========================================================================= */

#include <stdio.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <ctype.h>

#include <atalk/boolean.h>

#define LOGGER_C
#include <atalk/logger.h>
#undef LOGGER_C

#define OPEN_LOGS_AS_UID 0

#define COUNT_ARRAY(array) (sizeof((array))/sizeof((array)[0]))

/* =========================================================================
   Config
   ========================================================================= */

/* Main log config container, must be globally visible */
log_config_t log_config = {
    0,                  /* Initialized ? 0 = no */
    0,                  /* No filelogging setup yet */
    {0},                /* processname */
    0,                  /* syslog opened ? */
    logfacility_daemon,         /* syslog facility to use */
    logoption_ndelay|logoption_pid, /* logging options for syslog */
    0                               /* log level for syslog */
};

/* Default log config: log nothing to files.
   0:    not set individually
   NULL: Name of file
   -1:   logfiles fd
   0:   Log Level
   0:    Display options */
#define DEFAULT_LOG_CONFIG {0, NULL, -1, 0, 0}

filelog_conf_t file_configs[logtype_end_of_list_marker] = {
    DEFAULT_LOG_CONFIG, /* logtype_default */
    DEFAULT_LOG_CONFIG, /* logtype_core */
    DEFAULT_LOG_CONFIG, /* logtype_logger */
    DEFAULT_LOG_CONFIG, /* logtype_cnid */
    DEFAULT_LOG_CONFIG, /* logtype_afpd */
    DEFAULT_LOG_CONFIG, /* logtype_atalkd */
    DEFAULT_LOG_CONFIG, /* logtype_papd */
    DEFAULT_LOG_CONFIG  /* logtype_uams */
};

/* These are used by the LOG macro to store __FILE__ and __LINE__ */
char *log_src_filename;
int  log_src_linenumber;

/* Array to store text to list given a log type */
static const char *arr_logtype_strings[] =  LOGTYPE_STRING_IDENTIFIERS;
static const int num_logtype_strings = COUNT_ARRAY(arr_logtype_strings);

/* Array for charachters representing log severity in the log file */
static const char arr_loglevel_chars[] = {'-','S', 'E', 'W', 'N', 'I', 'D'};
static const int num_loglevel_chars = COUNT_ARRAY(arr_loglevel_chars);

static const char *arr_loglevel_strings[] = LOGLEVEL_STRING_IDENTIFIERS;
static const int num_loglevel_strings = COUNT_ARRAY(arr_loglevel_strings);

/* =========================================================================
   Internal function definitions
   ========================================================================= */

/*
 * If filename == NULL its for syslog logging, otherwise its for file-logging.
 * "unsetuplog" calls with loglevel == NULL.
 * loglevel == NULL means:
 *    if logtype == default
 *       disable logging
 *    else
 *       set to default logging
 */

/* -[un]setuplog <logtype> <loglevel> [<filename>]*/
static void setuplog_internal(const char *logtype, const char *loglevel, const char *filename)
{
    int typenum, levelnum;

    /* Parse logtype */
    for( typenum=0; typenum < num_logtype_strings; typenum++) {
        if (strcasecmp(logtype, arr_logtype_strings[typenum]) == 0)
            break;
    }
    if (typenum >= num_logtype_strings) {
        return;
    }

    /* Parse loglevel */
    if (loglevel == NULL) {
        levelnum = 0;
    } else {
        for(levelnum=1; levelnum < num_loglevel_strings; levelnum++) {
            if (strcasecmp(loglevel, arr_loglevel_strings[levelnum]) == 0)
                break;
        }
        if (levelnum >= num_loglevel_strings) {
            return;
        }
    }

    /* is this a syslog setup or a filelog setup ? */
    if (filename == NULL) {
        /* must be syslog */
        syslog_setup(levelnum, 0,
                     log_config.syslog_display_options,
                     log_config.facility);
    } else {
        /* this must be a filelog */
        log_setup(filename, levelnum, typenum);
    }

    return;
}

static void generate_message_details(char *message_details_buffer,
                                     int message_details_buffer_length,
                                     int display_options,
                                     enum loglevels loglevel, enum logtypes logtype)
{
    char   *ptr = message_details_buffer;
    int    templen;
    int    len = message_details_buffer_length;
    struct timeval tv;

    *ptr = 0;

    /* Print time */
    gettimeofday(&tv, NULL);
    strftime(ptr, len, "%b %d %H:%M:%S.", localtime(&tv.tv_sec));
    templen = strlen(ptr);
    len -= templen;
    ptr += templen;

    templen = snprintf(ptr, len, "%06u ", (int)tv.tv_usec);
    len -= templen;
    ptr += templen;

    /* Process name */
    strncpy(ptr, log_config.processname, len);
    templen = strlen(ptr);
    len -= templen;
    ptr += templen;

    /* PID */
    pid_t pid = getpid();
    templen = snprintf(ptr, len, "[%d]", pid);
    len -= templen;
    ptr += templen;

    /* Source info ? */
    if ( ! (display_options & logoption_nsrcinfo)) {
        char *basename = strrchr(log_src_filename, '/');
        if (basename)
            templen = snprintf(ptr, len, " {%s:%d}", basename + 1, log_src_linenumber);
        else
            templen = snprintf(ptr, len, " {%s:%d}", log_src_filename, log_src_linenumber);
        if (templen >= len)
            return;
        len -= templen;
        ptr += templen;
    }

    /* Errorlevel */
    if (loglevel >= (num_loglevel_chars - 1))
        templen = snprintf(ptr, len,  " (D%d:", loglevel - 1);
    else
        templen = snprintf(ptr, len, " (%c:", arr_loglevel_chars[loglevel]);
    len -= templen;
    ptr += templen;

    /* Errortype */
    if (logtype<num_logtype_strings) {
        templen = snprintf(ptr, len, "%s", arr_logtype_strings[logtype]);
        len -= templen;
        ptr += templen;
    }

    strncat(ptr, "): ", len);
}

int get_syslog_equivalent(enum loglevels loglevel)
{
    switch (loglevel)
    {
        /* The question is we know how bad it is for us,
           but how should that translate in the syslogs?  */
    case 1: /* severe */
        return LOG_ERR;
    case 2: /* error */
        return LOG_ERR;
    case 3: /* warning */
        return LOG_WARNING;
    case 4: /* note */
        return LOG_NOTICE;
    case 5: /* information */
        return LOG_INFO;
    default: /* debug */
        return LOG_DEBUG;
    }
}

/* =========================================================================
   Global function definitions
   ========================================================================= */

void log_init(void)
{
#ifdef LOGFILEPATH
    log_setup(LOGFILEPATH, log_note, logtype_default);
#else
    syslog_setup(log_note, 0,
                 log_config.syslog_display_options,
                 log_config.facility);
#endif
}

void log_setup(const char *filename, enum loglevels loglevel, enum logtypes logtype)
{
    uid_t process_uid;

    if (loglevel == 0) {
        /* Disable */
        if (file_configs[logtype].set) {
            if (file_configs[logtype].filename) {
                free(file_configs[logtype].filename);
                file_configs[logtype].filename = NULL;
            }
            close(file_configs[logtype].fd);
            file_configs[logtype].fd = -1;
            file_configs[logtype].level = 0;
            file_configs[logtype].set = 0;

            /* if disabling default also set all "default using" levels to 0 */
            if (logtype == logtype_default) {
                while (logtype != logtype_end_of_list_marker) {
                    if ( ! (file_configs[logtype].set))
                        file_configs[logtype].level = 0;
                    logtype++;
                }
            }
        }

        return;
    }

    /* Safety check */
    if (NULL == filename)
        return;

    /* Resetting existing config ? */
    if (file_configs[logtype].set && file_configs[logtype].filename) {
        free(file_configs[logtype].filename);
        file_configs[logtype].filename = NULL;
        close(file_configs[logtype].fd);
        file_configs[logtype].fd = -1;
        file_configs[logtype].level = 0;
        file_configs[logtype].set = 0;
    }

    /* Set new values */
    file_configs[logtype].filename = strdup(filename);
    file_configs[logtype].level = loglevel;


    /* Open log file as OPEN_LOGS_AS_UID*/
    process_uid = geteuid();
    if (process_uid)
        seteuid(OPEN_LOGS_AS_UID);
    file_configs[logtype].fd = open( file_configs[logtype].filename,
                                     O_CREAT | O_WRONLY | O_APPEND,
                                     S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (process_uid)
        seteuid(process_uid);

    /* Check for error opening/creating logfile */
    if (-1 == file_configs[logtype].fd) {
        free(file_configs[logtype].filename);
        file_configs[logtype].filename = NULL;
        file_configs[logtype].level = -1;
        file_configs[logtype].set = 0;
        return;
    }

    fcntl(file_configs[logtype].fd, F_SETFD, FD_CLOEXEC);
    file_configs[logtype].set = 1;
    log_config.filelogging = 1;
    log_config.inited = 1;

    /* Here's how we make it possible to LOG to a logtype like "logtype_afpd" */
    /* which then uses the default logtype setup if it isn't setup itself: */
    /* we just copy the loglevel from default to all logtypes that are not setup. */
    /* In "make_log_entry" we then check for the logtypes if they arent setup */
    /* and use default then. We must provide accessible values for all logtypes */
    /* in order to make it easy and fast to check the loglevels in the LOG macro! */

    if (logtype == logtype_default) {
        while (logtype != logtype_end_of_list_marker) {
            if ( ! (file_configs[logtype].set))
                file_configs[logtype].level = loglevel;
            logtype++;
        }
        logtype = logtype_default;
    }

    LOG(log_debug, logtype_logger, "Setup file logging: type: %s, level: %s, file: %s",
        arr_logtype_strings[logtype], arr_loglevel_strings[loglevel], file_configs[logtype].filename);
}

/* logtype is ignored, it's just one for all */
void syslog_setup(int loglevel, enum logtypes logtype _U_,
                  int display_options, int facility)
{
    log_config.syslog_level = loglevel;
    log_config.syslog_display_options = display_options;
    log_config.facility = facility;

    log_config.inited = 1;

    LOG(log_note, logtype_logger, "Set syslog logging to level: %s",
        arr_loglevel_strings[loglevel]);
}

void log_close()
{
}

/* This function sets up the processname */
void set_processname(const char *processname)
{
    strncpy(log_config.processname, processname, 15);
    log_config.processname[15] = 0;
}

/* -------------------------------------------------------------------------
   make_log_entry has 1 main flaws:
   The message in its entirity, must fit into the tempbuffer.
   So it must be shorter than MAXLOGSIZE
   ------------------------------------------------------------------------- */
void make_log_entry(enum loglevels loglevel, enum logtypes logtype,
                    char *message, ...)
{
    /* fn is not reentrant but is used in signal handler
     * with LOGGER it's a little late source name and line number
     * are already changed. */
    static int inlog = 0;
    int fd, len;
    char temp_buffer[MAXLOGSIZE];
    char log_details_buffer[MAXLOGSIZE];
    va_list args;
    struct iovec iov[2];

    if (inlog)
        return;
    inlog = 1;

    /* Initialise the Messages */
    va_start(args, message);
    len = vsnprintf(temp_buffer, MAXLOGSIZE - 1, message, args);
    va_end(args);

    /* Append \n */
    if (len >= MAXLOGSIZE)
        /* vsnprintf hit the buffer size*/
        temp_buffer[MAXLOGSIZE-2] = '\n';
    else {
        temp_buffer[len] = '\n';
        temp_buffer[len+1] = 0;
    }

    generate_message_details(log_details_buffer, sizeof(log_details_buffer),
                             file_configs[logtype].set ?
                             file_configs[logtype].display_options :
                             file_configs[logtype_default].display_options,
                             loglevel, logtype);

    /* Check if requested logtype is setup */
    if (file_configs[logtype].set)
        /* Yes */
        fd = file_configs[logtype].fd;
    else
        /* No: use default */
        fd = file_configs[logtype_default].fd;

    /* If default wasnt setup its fd is -1 */
    if (fd >= 0) {
        iov[0].iov_base = log_details_buffer;
        iov[0].iov_len = strlen(log_details_buffer);
        iov[1].iov_base = temp_buffer;
        iov[1].iov_len = strlen(temp_buffer);
        writev( fd,  iov, 2);
    }

    inlog = 0;
}

/* Called by the LOG macro for syslog messages */
void make_syslog_entry(enum loglevels loglevel, enum logtypes logtype, char *message, ...)
{
    va_list args;
    char log_buffer[MAXLOGSIZE];
    /* fn is not reentrant but is used in signal handler
     * with LOGGER it's a little late source name and line number
     * are already changed.
     */
    static int inlog = 0;

    if (inlog)
        return;
    inlog = 1;

    if ( ! (log_config.syslog_opened) ) {
        openlog(log_config.processname, log_config.syslog_display_options,
                log_config.facility);
        log_config.syslog_opened = 1;
    }

    /* Initialise the Messages */
    va_start(args, message);
    vsnprintf(log_buffer, sizeof(log_buffer), message, args);
    va_end(args);

    syslog(get_syslog_equivalent(loglevel), "%s", log_buffer);

    inlog = 0;
}

void setuplog(const char *logstr)
{
    char *ptr, *ptrbak, *logtype, *loglevel, *filename;
    ptr = strdup(logstr);
    ptrbak = ptr;

    /* logtype */
    logtype = ptr;

    /* get loglevel */
    ptr = strpbrk(ptr, " \t");
    if (ptr) {
        *ptr++ = 0;
        while (*ptr && isspace(*ptr))
            ptr++;
        loglevel = ptr;

        /* get filename */
        ptr = strpbrk(ptr, " \t");
        if (ptr) {
            *ptr++ = 0;
            while (*ptr && isspace(*ptr))
                ptr++;
        }
        filename = ptr;
    }

    /* finally call setuplog, filename can be NULL */
    setuplog_internal(logtype, loglevel, filename);

    free(ptrbak);
}

void unsetuplog(const char *logstr)
{
    char *str, *logtype, *filename;

    str = strdup(logstr);

    /* logtype */
    logtype = str;

    /* get filename, can be NULL */
    strtok(str, " \t");
    filename = strtok(NULL, " \t");

    /* finally call setuplog, filename can be NULL */
    setuplog_internal(str, NULL, filename);

    free(str);
}
