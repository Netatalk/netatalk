
#ifndef _ATALK_LOGGER_H
#define _ATALK_LOGGER_H 1

#include <atalk/boolean.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define MAXLOGSIZE 512

enum loglevels {
  log_severe   = 0,
  log_error    = 10,
  log_warning  = 20,
  log_note     = 30,
  log_info     = 40,
  log_debug    = 50
};
#define LOGLEVEL_STRING_IDENTIFIERS { \
  "LOG_SEVERE",                       \
  "LOG_ERROR",                        \
  "LOG_WARN",                         \
  "LOG_NOTE",                         \
  "LOG_INFO",                         \
  "LOG_DEBUG"}                        

/* this is the enum specifying all availiable logtypes */
enum logtypes {
  logtype_default,
  logtype_core,
  logtype_logger,
  logtype_cnid,
  logtype_afpd,

  logtype_end_of_list_marker  /* don't put any logtypes after this */
};

/* these are the string identifiers corresponding to each logtype */
#define LOGTYPE_STRING_IDENTIFIERS { \
  "Default",                         \
  "Core",                            \
  "Logger",                          \
  "CNID",                            \
  "AFPDaemon",                       \
                                     \
  "end_of_list_marker"}              \

/* Display Option flags. */
/* redefine these so they can don't interfeer with syslog */
/* these can be used in standard logging too */
#define logoption_pid         0x01    /* log the pid with each message */
#define logoption_cons        0x02    /* log on the console if errors in sending */
#define logoption_ndelay      0x08    /* don't delay open */
#define logoption_perror      0x20    /* log to stderr as well */
#define logoption_nfile       0x40    /* don't log the file name that called the log */
#define logoption_nline       0x80    /* don't log the line number from where the log was called */

/* facility codes */
/* redefine these so they can don't interfeer with syslog */
#define logfacility_user        (1<<3)  /* random user-level messages */
#define logfacility_mail        (2<<3)  /* mail system */
#define logfacility_daemon      (3<<3)  /* system daemons */
#define logfacility_auth        (4<<3)  /* security/authorization messages */
#define logfacility_syslog      (5<<3)  /* messages generated internally by syslogd */
#define logfacility_lpr         (6<<3)  /* line printer subsystem */
#define logfacility_authpriv    (10<<3) /* security/authorization messages (private) */
#define logfacility_ftp         (11<<3) /* ftp daemon */

/* Setup the log filename and the loglevel, and the type of log it is. */
/* setup the internal variables used by the logger (called automatically) */
void log_init();

bool log_setup(char *filename, enum loglevels loglevel, enum logtypes logtype, int display_options);

/* Setup the Level and type of log that will be logged to syslog. */
void syslog_setup(enum loglevels loglevel, enum logtypes logtype, int display_options, int facility);

void setuplog(char *logsource, char *logtype, char *loglevel, char *filename);

/* finish up and close the logs */
void log_close();

/* This function sets up the ProcessName */
void set_processname(char *processname);

/* Log a Message */
void make_log_entry(enum loglevels loglevel, enum logtypes logtype, 
             char *message, ...);

#ifndef DISABLE_LOGGER
typedef void(*make_log_func)
       (enum loglevels loglevel, enum logtypes logtype, char *message, ...);
make_log_func set_log_location(char *srcfilename, int srclinenumber);

void LoadProccessNameFromProc();

#define LOG set_log_location(__FILE__, __LINE__)
#else /* DISABLE_LOGGER */
/* if the logger is disabled the rest is a bit futile */
#define LOG make_log_entry
#endif /* DISABLE_LOGGER */

#endif
