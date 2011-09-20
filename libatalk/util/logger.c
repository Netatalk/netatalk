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
#include <errno.h>
#include <stdbool.h>

#include <atalk/util.h>
#include <atalk/logger.h>

#define OPEN_LOGS_AS_UID 0

#define COUNT_ARRAY(array) (sizeof((array))/sizeof((array)[0]))

#define MAXLOGSIZE 512

#define LOGLEVEL_STRING_IDENTIFIERS { \
  "LOG_NOTHING",                      \
  "LOG_SEVERE",                       \
  "LOG_ERROR",                        \
  "LOG_WARN",                         \
  "LOG_NOTE",                         \
  "LOG_INFO",                         \
  "LOG_DEBUG",                        \
  "LOG_DEBUG6",                       \
  "LOG_DEBUG7",                       \
  "LOG_DEBUG8",                       \
  "LOG_DEBUG9",                       \
  "LOG_MAXDEBUG"}                        

/* these are the string identifiers corresponding to each logtype */
#define LOGTYPE_STRING_IDENTIFIERS { \
  "Default",                         \
  "Logger",                          \
  "CNID",                            \
  "AFPDaemon",                       \
  "DSI",                             \
  "ATalkDaemon",                     \
  "PAPDaemon",                       \
  "UAMS",                            \
  "end_of_list_marker"}              \

/* =========================================================================
   Config
   ========================================================================= */

/* Main log config container */
log_config_t log_config = { 0 };

/* Default log config: log nothing to files.
   0:               set ?
   0:               syslog ?
   -1:              logfiles fd
   log_none:        no logging by default
   0:               Display options */
#define DEFAULT_LOG_CONFIG {0, 0, -1, log_none, 0}

UAM_MODULE_EXPORT logtype_conf_t type_configs[logtype_end_of_list_marker] = {
    DEFAULT_LOG_CONFIG, /* logtype_default */
    DEFAULT_LOG_CONFIG, /* logtype_logger */
    DEFAULT_LOG_CONFIG, /* logtype_cnid */
    DEFAULT_LOG_CONFIG, /* logtype_afpd */
    DEFAULT_LOG_CONFIG, /* logtype_dsi */
    DEFAULT_LOG_CONFIG, /* logtype_atalkd */
    DEFAULT_LOG_CONFIG, /* logtype_papd */
    DEFAULT_LOG_CONFIG /* logtype_uams */
};

/* We use this in order to track the last n log messages in order to prevent flooding */
#define LOG_FLOODING_MINCOUNT 5 /* this controls after how many consecutive messages must be detected
                                   before we start to hide them */
#define LOG_FLOODING_MAXCOUNT 1000 /* this controls after how many consecutive messages we force a 
                                      "repeated x times" message */
#define LOG_FLOODING_ARRAY_SIZE 3 /* this contols how many messages in flow we track */
struct log_flood_entry {
    int count;
    unsigned int hash;
};
static struct log_flood_entry log_flood_array[LOG_FLOODING_ARRAY_SIZE];
static int log_flood_entries;

/* These are used by the LOG macro to store __FILE__ and __LINE__ */
static const char *log_src_filename;
static int  log_src_linenumber;

/* Array to store text to list given a log type */
static const char *arr_logtype_strings[] =  LOGTYPE_STRING_IDENTIFIERS;
static const unsigned int num_logtype_strings = COUNT_ARRAY(arr_logtype_strings);

/* Array for charachters representing log severity in the log file */
static const char arr_loglevel_chars[] = {'-','S', 'E', 'W', 'N', 'I', 'D'};
static const unsigned int num_loglevel_chars = COUNT_ARRAY(arr_loglevel_chars);

static const char *arr_loglevel_strings[] = LOGLEVEL_STRING_IDENTIFIERS;
static const unsigned int num_loglevel_strings = COUNT_ARRAY(arr_loglevel_strings);

/* =========================================================================
   Internal function definitions
   ========================================================================= */

/* Hash a log message */
static unsigned int hash_message(const char *message)
{
    const char *p = message;
    unsigned int hash = 0, i = 7;

    while (*p) {
        hash += *p * i;
        i++;
        p++;
    }
    return hash;
}

/*
 * If filename == NULL its for syslog logging, otherwise its for file-logging.
 * "unsetuplog" calls with loglevel == NULL.
 * loglevel == NULL means:
 *    if logtype == default
 *       disable logging
 *    else
 *       set to default logging
 */
static void setuplog_internal(const char *loglevel, const char *logtype, const char *filename)
{
    unsigned int typenum, levelnum;

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
        syslog_setup(levelnum,
                     typenum,
                     logoption_ndelay | logoption_pid,
                     logfacility_daemon);
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
    pid_t  pid;

    *ptr = 0;

    /* Print time */
    gettimeofday(&tv, NULL);
    strftime(ptr, len, "%b %d %H:%M:%S.", localtime(&tv.tv_sec));
    templen = strlen(ptr);
    len -= templen;
    ptr += templen;

    templen = snprintf(ptr, len, "%06u ", (int)tv.tv_usec);
    if (templen == -1 || templen >= len)
        return;
        
    len -= templen;
    ptr += templen;

    /* Process name &&  PID */
    pid = getpid();
    templen = snprintf(ptr, len, "%s[%d]", log_config.processname, pid);
    if (templen == -1 || templen >= len)
        return;
    len -= templen;
    ptr += templen;

    /* Source info ? */
    if ( ! (display_options & logoption_nsrcinfo)) {
        char *basename = strrchr(log_src_filename, '/');
        if (basename)
            templen = snprintf(ptr, len, " {%s:%d}", basename + 1, log_src_linenumber);
        else
            templen = snprintf(ptr, len, " {%s:%d}", log_src_filename, log_src_linenumber);
        if (templen == -1 || templen >= len)
            return;
        len -= templen;
        ptr += templen;
    }

    /* Errorlevel */
    if (loglevel >= (num_loglevel_chars - 1))
        templen = snprintf(ptr, len,  " (D%d:", loglevel - 1);
    else
        templen = snprintf(ptr, len, " (%c:", arr_loglevel_chars[loglevel]);

    if (templen == -1 || templen >= len)
        return;
    len -= templen;
    ptr += templen;

    /* Errortype */
    if (logtype<num_logtype_strings) {
        templen = snprintf(ptr, len, "%s", arr_logtype_strings[logtype]);
        if (templen == -1 || templen >= len)
            return;
        len -= templen;
        ptr += templen;
    }
    
    strncat(ptr, "): ", len);
    ptr[len -1] = 0;
}

static int get_syslog_equivalent(enum loglevels loglevel)
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
    syslog_setup(log_info,
                 logtype_default,
                 logoption_ndelay | logoption_pid,
                 logfacility_daemon);
}

void log_setup(const char *filename, enum loglevels loglevel, enum logtypes logtype)
{
    uid_t process_uid;

    if (loglevel == 0) {
        /* Disable */
        if (type_configs[logtype].set) {
            if (type_configs[logtype].fd != -1)
                close(type_configs[logtype].fd);
            type_configs[logtype].fd = -1;
            type_configs[logtype].level = -1;
            type_configs[logtype].set = false;

            /* if disabling default also set all "default using" levels to 0 */
            if (logtype == logtype_default) {
                while (logtype != logtype_end_of_list_marker) {
                    if ( ! (type_configs[logtype].set))
                        type_configs[logtype].level = -1;
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
    if (type_configs[logtype].set) {
        if (type_configs[logtype].fd != -1)
            close(type_configs[logtype].fd);
        type_configs[logtype].fd = -1;
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
    } else if (strcmp(filename + strlen(filename) - 6, "XXXXXX") == 0) {
        char *tmp = strdup(filename);
        type_configs[logtype].fd = mkstemp(tmp);
        free(tmp);

    } else {
        process_uid = geteuid();
        if (process_uid) {
            if (seteuid(OPEN_LOGS_AS_UID) == -1) {
                process_uid = 0;
            }
        }
        type_configs[logtype].fd = open(filename,
                                        O_CREAT | O_WRONLY | O_APPEND,
                                        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (process_uid) {
            if (seteuid(process_uid) == -1) {
                LOG(log_error, logtype_logger, "can't seteuid back %s", strerror(errno));
                exit(EXITERR_SYS);
            }
        }
    }

    /* Check for error opening/creating logfile */
    if (type_configs[logtype].fd == -1) {
        type_configs[logtype].level = -1;
        type_configs[logtype].set = false;
        return;
    }

    fcntl(type_configs[logtype].fd, F_SETFD, FD_CLOEXEC);
    type_configs[logtype].set = true;
    log_config.inited = true;

    /* Here's how we make it possible to LOG to a logtype like "logtype_afpd" */
    /* which then uses the default logtype setup if it isn't setup itself: */
    /* we just copy the loglevel from default to all logtypes that are not setup. */
    /* In "make_log_entry" we then check for the logtypes if they arent setup */
    /* and use default then. We must provide accessible values for all logtypes */
    /* in order to make it easy and fast to check the loglevels in the LOG macro! */

    if (logtype == logtype_default) {
        int typeiter = 0;
        while (typeiter != logtype_end_of_list_marker) {
            if ( ! (type_configs[typeiter].set))
                type_configs[typeiter].level = loglevel;
            typeiter++;
        }
    }

    LOG(log_debug, logtype_logger, "Setup file logging: type: %s, level: %s, file: %s",
        arr_logtype_strings[logtype], arr_loglevel_strings[loglevel], filename);
}

/* Setup syslog logging */
void syslog_setup(int loglevel, enum logtypes logtype, int display_options, int facility)
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
            if ( ! (type_configs[typeiter].set)) {
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

void log_close(void)
{
}

/* This function sets up the processname */
void set_processname(const char *processname)
{
    strncpy(log_config.processname, processname, 15);
    log_config.processname[15] = 0;
}

/* Called by the LOG macro for syslog messages */
static void make_syslog_entry(enum loglevels loglevel, enum logtypes logtype _U_, char *message)
{
    if ( !log_config.syslog_opened ) {
        openlog(log_config.processname,
                log_config.syslog_display_options,
                log_config.syslog_facility);
        log_config.syslog_opened = true;
    }

    syslog(get_syslog_equivalent(loglevel), "%s", message);
}

/* -------------------------------------------------------------------------
   make_log_entry has 1 main flaws:
   The message in its entirity, must fit into the tempbuffer.
   So it must be shorter than MAXLOGSIZE
   ------------------------------------------------------------------------- */
void make_log_entry(enum loglevels loglevel, enum logtypes logtype,
                    const char *file, int line, char *message, ...)
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

    if (!log_config.inited) {
      log_init();
    }
    
    if (type_configs[logtype].syslog) {
        if (type_configs[logtype].level >= loglevel) {
            /* Initialise the Messages and send it to syslog */
            va_start(args, message);
            vsnprintf(temp_buffer, MAXLOGSIZE -1, message, args);
            va_end(args);
            temp_buffer[MAXLOGSIZE -1] = 0;
            make_syslog_entry(loglevel, logtype, temp_buffer);
        }
        inlog = 0;
        return;
    }

    /* logging to a file */

    log_src_filename = file;
    log_src_linenumber = line;

    /* Check if requested logtype is setup */
    if (type_configs[logtype].set)
        /* Yes */
        fd = type_configs[logtype].fd;
    else
        /* No: use default */
        fd = type_configs[logtype_default].fd;

    if (fd < 0) {
        /* no where to send the output, give up */
        goto exit;
    }

    /* Initialise the Messages */
    va_start(args, message);
    len = vsnprintf(temp_buffer, MAXLOGSIZE -1, message, args);
    va_end(args);

    /* Append \n */
    if (len ==-1 || len >= MAXLOGSIZE -1) {
        /* vsnprintf hit the buffer size*/
        temp_buffer[MAXLOGSIZE-2] = '\n';
        temp_buffer[MAXLOGSIZE-1] = 0;
    }
    else {
        temp_buffer[len] = '\n';
        temp_buffer[len+1] = 0;
    }

    if (type_configs[logtype].level >= log_debug)
        goto log; /* bypass flooding checks */

    /* Prevent flooding: hash the message and check if we got the same one recently */
    int hash = hash_message(temp_buffer) + log_src_linenumber;

    /* Search for the same message by hash */
    for (int i = log_flood_entries - 1; i >= 0; i--) {
        if (log_flood_array[i].hash == hash) {

            /* found same message */
            log_flood_array[i].count++;

            /* Check if that message has reached LOG_FLOODING_MAXCOUNT */
            if (log_flood_array[i].count >= LOG_FLOODING_MAXCOUNT) {
                /* yes, log it and remove from array */

                /* reusing log_details_buffer */
                sprintf(log_details_buffer, "message repeated %i times\n",
                        LOG_FLOODING_MAXCOUNT - 1);
                write(fd, log_details_buffer, strlen(log_details_buffer));

                if ((i + 1) == LOG_FLOODING_ARRAY_SIZE) {
                    /* last array element, just decrement count */
                    log_flood_entries--;
                    goto exit;
                }
                /* move array elements down */
                for (int j = i + 1; j != LOG_FLOODING_ARRAY_SIZE ; j++)
                    log_flood_array[j-1] = log_flood_array[j];
                log_flood_entries--;
            }

            if (log_flood_array[i].count < LOG_FLOODING_MINCOUNT)
                /* log it */
                goto log;
            /* discard it */
            goto exit;
        } /* if */
    }  /* for */

    /* No matching message found, add this message to array*/
    if (log_flood_entries == LOG_FLOODING_ARRAY_SIZE) {
        /* array is full, discard oldest entry printing "message repeated..." if count > 1 */
        if (log_flood_array[0].count >= LOG_FLOODING_MINCOUNT) {
            /* reusing log_details_buffer */
            sprintf(log_details_buffer, "message repeated %i times\n",
                    log_flood_array[0].count - LOG_FLOODING_MINCOUNT + 1);
            write(fd, log_details_buffer, strlen(log_details_buffer));
        }
        for (int i = 1; i < LOG_FLOODING_ARRAY_SIZE; i++) {
            log_flood_array[i-1] = log_flood_array[i];
        }
        log_flood_entries--;
    }
    log_flood_array[log_flood_entries].count = 1;
    log_flood_array[log_flood_entries].hash = hash;
    log_flood_entries++;

log:
    if ( ! log_config.console) {
        generate_message_details(log_details_buffer, sizeof(log_details_buffer),
                                 type_configs[logtype].set ?
                                     type_configs[logtype].display_options :
                                     type_configs[logtype_default].display_options,
                                 loglevel, logtype);

        /* If default wasnt setup its fd is -1 */
        iov[0].iov_base = log_details_buffer;
        iov[0].iov_len = strlen(log_details_buffer);
        iov[1].iov_base = temp_buffer;
        iov[1].iov_len = strlen(temp_buffer);
        writev( fd,  iov, 2);
    } else {
        write(fd, temp_buffer, strlen(temp_buffer));
    }

exit:
    inlog = 0;
}


void setuplog(const char *logstr)
{
    char *ptr, *ptrbak, *logtype, *loglevel = NULL, *filename = NULL;
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
        if (filename && *filename == 0)
            filename = NULL;
    }

    /* finally call setuplog, filename can be NULL */
    setuplog_internal(loglevel, logtype, filename);

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
    setuplog_internal(NULL, str, filename);

    free(str);
}
