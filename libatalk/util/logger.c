#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* =========================================================================

       logger.c is part of the utils section in the libatalk library, 
        which is part of the netatalk project.  

       logger.c was written by Simon Bazley (sibaz@sibaz.com)

       I believe libatalk is released under the L/GPL licence.  

       Just incase, it is, thats the licence I'm applying to this file.

       Netatalk 2001 (c)

   ==========================================================================

       Logger.c is intended as an alternative to syslog for logging

       ---------------------------------------------------------------

   The initial plan is to create a structure for general information needed
    to log to a file.  

   Initally I'll hard code the neccesary stuff to start a log, this should
    probably be moved elsewhere when some code is written to read the log
    file locations from the config files.  

   As a more longterm idea, I'll code this so that the data struct can be
    duplicated to allow multiple concurrent log files, although this is 
    probably a recipe for wasted resources. 

   ========================================================================= */

#include <stdio.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include <atalk/boolean.h>
#include <atalk/logger.h>

#define COUNT_ARRAY(array) (sizeof((array))/sizeof((array)[0]))
#define NUMOF COUNT_ARRAY
#undef  KEEP_LOGFILES_OPEN
#define DO_SYSLOG
#define DO_FILELOG

#undef  DEBUG_OUTPUT_TO_SCREEN
#undef  CHECK_STAT_ON_NEW_FILES 
#undef  CHECK_ACCESS_ON_NEW_FILES 

/* =========================================================================
    External function declarations
   ========================================================================= */

/* setup the internal variables used by the logger (called automatically) */
void log_init();

/* Setup the log filename and the loglevel, and the type of log it is. */
bool log_setup(char *filename, enum loglevels loglevel, enum logtypes logtype, 
	       int display_options);

/* Setup the Level and type of log that will be logged to syslog. */
void syslog_setup(enum loglevels loglevel, enum logtypes logtype, 
		  int display_options, int facility);

/* finish up and close the logs */
void log_close();

/* This function sets up the processname */
void set_processname(char *processname);

/* Log a Message */
void make_log(enum loglevels loglevel, enum logtypes logtype, 
	      char *message, ...);
#ifndef DISABLE_LOGGER
make_log_func set_log_location(char *srcfilename, int srclinenumber);

/* ========================================================================= 
    Structure definitions
   ========================================================================= */

/* A structure containing object level stuff */
struct tag_log_file_data {
  char log_filename[PATH_MAX];  /* Name of file */
  FILE *log_file;      /* FILE pointer to file */
  enum loglevels   log_level;     /* Log Level to put in this file */
  int  display_options;
};

typedef struct tag_log_file_data log_file_data_pair[2];

/* A structure containg class level stuff */
struct tag_global_log_data {
  int   struct_size;

  char *temp_src_filename;
  int   temp_src_linenumber;
  char  processname[16];
  
  int   facility;
  char *log_file_directory;  /* Path of directory containing log files */
  log_file_data_pair **logs;
};

struct what_to_print_array {
  bool print_datetime;
  bool print_processname;
  bool print_pid;
  bool print_srcfile;
  bool print_srcline;
  bool print_errlevel;
  bool print_errtype;
};

/* =========================================================================
    Internal function declarations
   ========================================================================= */

void generate_message_details(char *message_details_buffer,
                              int message_details_buffer_length,
                              struct tag_log_file_data *log_struct,
                              enum loglevels loglevel, enum logtypes logtype);

int get_syslog_equivalent(enum loglevels loglevel);

static char *get_command_name(char *commandpath);

/* =========================================================================
    Instanciated data
   ========================================================================= */

/* A populated instance */

static log_file_data_pair default_log_file_data_pair = {
{
  log_filename:    "\0\0\0\0\0\0\0\0",
  log_file:        NULL,
  log_level:       log_debug,
  display_options: logoption_pid
},
{
  log_filename:     LOGFILEPATH,
  log_file:         NULL,
  log_level:        log_debug,
  display_options:  logoption_pid
}};

static log_file_data_pair *log_file_data_array[logtype_end_of_list_marker] = 
{&default_log_file_data_pair};

/* The class (populated) */
static struct tag_global_log_data global_log_data = {
  struct_size:         sizeof(struct tag_global_log_data),
  temp_src_filename:   NULL,
  temp_src_linenumber: 0,
  processname:         "",
  facility:            logfacility_daemon,
  log_file_directory:  "",
  logs:                NULL,
};

/* macro to get access to the array */
#define log_file_arr (global_log_data.logs)

/* Array to store text to list given a log type */
static const char * arr_logtype_strings[] =  LOGTYPE_STRING_IDENTIFIERS;
static const int num_logtype_strings = COUNT_ARRAY(arr_logtype_strings);

/* Array for charachters representing log severity in the log file */
static const char arr_loglevel_chars[] = {'S', 'E', 'W', 'N', 'I', 'D'};
static const int num_loglevel_chars = COUNT_ARRAY(arr_loglevel_chars);

static const char * arr_loglevel_strings[] = LOGLEVEL_STRING_IDENTIFIERS;
static const int num_loglevel_strings = COUNT_ARRAY(arr_loglevel_strings);

#else /* #ifndef DISABLE_LOGGER */
  char *disabled_logger_processname=NULL;
#endif /* DISABLE_LOGGER */
/* =========================================================================
    Global function definitions
   ========================================================================= */

#ifndef DISABLE_LOGGER

/* remember I'm keeping a copy of the actual char * Filename, so you mustn't
   delete it, until you've finished with the log.  Also you're responsible
   for deleting it when you have finished with it. */
void log_init()
{
  if (global_log_data.logs==NULL)
  {
    /* first check default_log_file_data_pair */

    /* next clear out the log_file_data_array */
    memset(log_file_data_array, 0, sizeof(log_file_data_array));
    /* now set default_log_file_data_pairs */
    log_file_data_array[0] = &default_log_file_data_pair;

    /* now setup the global_log_data struct */
    global_log_data.logs = log_file_data_array;
  }
}
#endif /* #ifndef DISABLE_LOGGER */

bool log_setup(char *filename, enum loglevels loglevel, enum logtypes logtype, 
	       int display_options)
{
#ifndef DISABLE_LOGGER

  struct stat statbuf;
  int firstattempt;
  int retval;
  gid_t gid;
  uid_t uid;
  int access;
  char lastchar[2];

  log_file_data_pair *logs;

  log_init();

  logs = log_file_arr[logtype];

  if (logs==NULL)
  {
    logs = (log_file_data_pair *)malloc(sizeof(log_file_data_pair));
    if (logs==NULL)
    {
      LOG(log_severe, logtype_logger, "can't calloc in log_setup");
    }
    else
    {
    /*
      memcpy(logs, log_file_arr[logtype_default], sizeof(log_file_data_pair));
     */
      log_file_arr[logtype] = logs;
      (*logs)[1].log_file = NULL;
    }
  }

  if ( ((*logs)[1].log_file == stdout) && ((*logs)[1].log_file != NULL) )
  {
    fclose((*logs)[1].log_file);
    (*logs)[1].log_file = NULL;
  }

  if (strlen(global_log_data.log_file_directory)>0)
  {
    lastchar[0] = global_log_data.
      log_file_directory[strlen(global_log_data.log_file_directory)-1];

    if (lastchar[0] == '/' || lastchar[0] == '\\' || lastchar[0] == ':')
      lastchar[0] = 0;
    else
      /* this should probably be a platform specific path separator */
      lastchar[0] = '/';

    lastchar[1] = 0;
  }
  else
    lastchar[0] = 0;
  
#ifdef DEBUG_OUTPUT_TO_SCREEN
  printf("filename is %s stored at location %p\n", (*logs)[1].log_filename, 
                                                   (*logs)[1].log_filename);
#endif /* DEBUG_OUTPUT_TO_SCREEN */
  if (filename == NULL)
  {
    strncpy((*logs)[1].log_filename, 
	    (*(log_file_arr[0]))[1].log_filename, PATH_MAX);
  }
  else
  {
    sprintf((*logs)[1].log_filename, "%s%s%s", 
	    global_log_data.log_file_directory,
	    lastchar, filename);
  }
  (*logs)[1].log_level    = loglevel;
  (*logs)[1].display_options = display_options;

#ifdef DEBUG_OUTPUT_TO_SCREEN
  printf("filename is %s stored at location %p\n", (*logs)[1].log_filename, 
                                                   (*logs)[1].log_filename);
#endif /* DEBUG_OUTPUT_TO_SCREEN */

#ifdef CHECK_STAT_ON_NEW_FILES
  uid = geteuid(); 
  gid = getegid();
  
#ifdef DEBUG_OUTPUT_TO_SCREEN
  printf("about to stat file %s\n", (*logs)[1].log_filename);
#endif
  firstattempt = stat((*logs)[1].log_filename, &statbuf);

  if (firstattempt == -1)
  {
#ifdef DEBUG_OUTPUT_TO_SCREEN
    printf("about to call Log with %d, %d, %s, %s\n", 
	   log_note, logtype_logger, 
           "can't stat Logfile", 
	   (*logs)[1].log_filename
	   );
#endif

    /* syslog(LOG_INFO, "stat failed"); */
    LOG(log_warning, logtype_logger, "stat fails on file %s",
                     (*logs)[1].log_filename);

    if (strlen(global_log_data.log_file_directory)>0)
    {
      retval = stat(global_log_data.log_file_directory, &statbuf);
      if (retval == -1)
      {
#ifdef DEBUG_OUTPUT_TO_SCREEN
        printf("can't stat dir either so I'm giving up\n");
#endif
        LOG(log_severe, logtype_logger, "can't stat directory %s either",
                         global_log_data.log_file_directory);
        return false;
      } 
    }
  }

#ifdef CHECK_ACCESS_ON_NEW_FILES
  access = ((statbuf.st_uid == uid)?(statbuf.st_mode & S_IWUSR):0) + 
           ((statbuf.st_gid == gid)?(statbuf.st_mode & S_IWGRP):0) +
           (statbuf.st_mode & S_IWOTH);

  if (access==0)
  {
#ifdef DEBUG_OUTPUT_TO_SCREEN
    printf("failing with %d, %d, %s, %s\n", log_note, logtype_logger,
           "can't access Logfile %s",    (*logs)[1].log_filename);
#endif

    LOG(log_note, logtype_logger, "can't access file %s", 
                     (*logs)[1].log_filename);
    return false;
  }
#endif /* CHECK_ACCESS_ON_NEW_FILES */
#endif /* CHECK_STAT_ON_NEW_FILES */
#ifdef KEEP_LOGFILES_OPEN
  if ((*logs)[1].log_file!=NULL)
    fclose((*logs)[1].log_file);

  (*logs)[1].log_file     = fopen((*logs)[1].log_filename, "at");
  if ((*logs)[1].log_file == NULL)
  {
    LOG(log_severe, logtype_logger, "can't open Logfile %s", 
	(*logs)[1].log_filename
	);
    return false;
  }
#endif

  LOG(log_info, logtype_logger, "Log setup complete");

#endif /* DISABLE_LOGGER */
  return true;
}


void syslog_setup(enum loglevels loglevel, enum logtypes logtype, 
		  int display_options, int facility)
{
#ifndef DISABLE_LOGGER
  log_file_data_pair *logs;

  log_init();

  logs = log_file_arr[logtype];

  if (logs==NULL)
  {
    logs = (log_file_data_pair *)malloc(sizeof(log_file_data_pair));    
    if (logs==NULL)
    {
      LOG(log_severe, logtype_logger, "can't calloc in log_setup");
    }
    else
    {
      memcpy(logs, log_file_arr[logtype_default], sizeof(log_file_data_pair));
      log_file_arr[logtype] = logs;
    }
  }

  (*logs)[0].log_file        = NULL;
  (*logs)[0].log_filename[0] = 0;
  (*logs)[0].log_level       = loglevel;
  (*logs)[0].display_options = display_options;
  global_log_data.facility   = facility;

  openlog(global_log_data.processname, (*logs)[0].display_options, 
	  global_log_data.facility);

  LOG(log_info, logtype_logger, "SysLog setup complete");
#else /* DISABLE_LOGGER */
/* behave like a normal openlog call */
  openlog(disabled_logger_processname, display_options, facility);
#endif /* DISABLE_LOGGER */
}

void log_close()
{
#ifndef DISABLE_LOGGER
  log_file_data_pair *logs;
  int n;

  LOG(log_info, logtype_logger, "Closing logs");

  for(n=(sizeof(log_file_arr)-1);n>0;n--)
  {
    logs = log_file_arr[n];
#ifdef KEEP_LOGFILES_OPEN
    if ((*logs)[1].log_file!=NULL)
      fclose((*logs)[1].log_file);
#endif /* KEEP_LOGFILES_OPEN */
    if (logs!=NULL)
    {
#ifdef DEBUG_OUTPUT_TO_SCREEN
      printf("Freeing log_data %d, stored at %p\n", n, logs);
      printf("\t(filename) %s\t(type) %s\n", (*logs)[1].log_filename, 
             ((n<num_logtype_strings)?arr_logtype_strings[n]:""));
#endif /* DEBUG_OUTPUT_TO_SCREEN */
      free(logs);
    }
    log_file_arr[n] = NULL;
  }
#ifdef DEBUG_OUTPUT_TO_SCREEN
      printf("Freeing log_data %d, stored at %p\n", n, log_file_arr[n]);
      printf("\t(filename) %s\t(type) %s\n", 
	     (*(log_file_arr[n]))[1].log_filename, 
             ((n<num_logtype_strings)?arr_logtype_strings[n]:"")
	     );
#endif /* DEBUG_OUTPUT_TO_SCREEN */
#endif /* DISABLE_LOGGER */

  closelog();
}

/* This function sets up the processname */
void set_processname(char *processname)
{
#ifndef DISABLE_LOGGER
  /* strncpy(global_log_data.processname, GetCommandName(processname), 15); */
  strncpy(global_log_data.processname, processname, 15);
  global_log_data.processname[15] = 0;
#else /* DISABLE_LOGGER */
  disabled_logger_processname = processname;
#endif /* DISABLE_LOGGER */
}

#ifndef DISABLE_LOGGER
/* This is called by the macro so set the location of the caller of Log */
make_log_func set_log_location(char *srcfilename, int srclinenumber)
{
#ifdef DEBUG_OUTPUT_TO_SCREEN
  printf("Setting Log Location\n");
#endif
  global_log_data.temp_src_filename = srcfilename;
  global_log_data.temp_src_linenumber = srclinenumber;

  return make_log_entry;
}
#endif /* DISABLE_LOGGER */

/* -------------------------------------------------------------------------
    MakeLog has 1 main flaws:
      The message in its entirity, must fit into the tempbuffer.  
      So it must be shorter than MAXLOGSIZE
   ------------------------------------------------------------------------- */
void make_log_entry(enum loglevels loglevel, enum logtypes logtype, 
		    char *message, ...)
{
  va_list args;
  char log_buffer[MAXLOGSIZE];
#ifndef DISABLE_LOGGER
  char log_details_buffer[MAXLOGSIZE];

  log_file_data_pair *logs;

  log_init();

  logs = log_file_arr[logtype];

  if (logs==NULL)
  {
    logs = log_file_arr[logtype_default];
  }
#ifdef DEBUG_OUTPUT_TO_SCREEN
  printf("Making Log\n");
#endif
  
#endif /* DISABLE_LOGGER */

  /* Initialise the Messages */
  va_start(args, message);

  vsnprintf(log_buffer, sizeof(log_buffer), message, args);

  /* Finished with args for now */
  va_end(args);

#ifdef DISABLE_LOGGER
  syslog(get_syslog_equivalent(loglevel), "%s", log_buffer);
#else /* DISABLE_LOGGER */

#ifdef DO_SYSLOG
  /* check if sysloglevel is high enough */
  if ((*logs)[0].log_level>=loglevel)
  {
    int sysloglevel = get_syslog_equivalent(loglevel);

    generate_message_details(log_details_buffer, sizeof(log_details_buffer),
                              &(*logs)[0], loglevel, logtype);

#ifdef DEBUG_OUTPUT_TO_SCREEN
    printf("About to log %s %s\n", log_details_buffer, log_buffer);
    printf("about to do syslog\n");
    printf("done onw syslog\n");
#endif
    syslog(sysloglevel, "%s: %s", log_details_buffer, log_buffer);
    /* 
       syslog(sysloglevel, "%s:%s: %s", log_levelString, 
              log_typeString, LogBuffer); 
     */
  }
#endif

#ifdef DO_FILELOG
#ifdef DEBUG_OUTPUT_TO_SCREEN
  printf("about to do the filelog\n");
#endif
  /* check if log_level is high enough */
  if ((*logs)[1].log_level>=loglevel) {    
 
#ifdef DEBUG_OUTPUT_TO_SCREEN
    printf("Open the Log, FILE* is %p\n", (*logs)[1].log_file);
#endif
    /* if log isn't open, open it */
    if ((*logs)[1].log_file==NULL) {
#ifdef DEBUG_OUTPUT_TO_SCREEN
      printf("Opening the Log, filename is %s\n", (*logs)[1].log_filename);
#endif
      (*logs)[1].log_file     = fopen((*logs)[1].log_filename, "at");
      if ((*logs)[1].log_file == NULL)
      {
        (*logs)[1].log_file = stdout;
        LOG(log_severe, logtype_logger, "can't open Logfile %s", 
	    (*logs)[1].log_filename
	    );
        return;
      }
    }
    generate_message_details(log_details_buffer, sizeof(log_details_buffer),
                             &(*logs)[1], loglevel, logtype);

#ifdef DEBUG_OUTPUT_TO_SCREEN
    printf("Files open, lets log\n");
    printf("FILE* is %p\n", (*logs)[1].log_file);
    printf("%s: %s\n", log_details_buffer, log_buffer);
#endif

    fprintf((*logs)[1].log_file, "%s: %s\n", log_details_buffer, log_buffer);

#ifndef KEEP_LOGFILES_OPEN
    if ((*logs)[1].log_file != stdout)
    {
#ifdef DEBUG_OUTPUT_TO_SCREEN
      printf("Closing %s\n", (*logs)[1].log_filename);
#endif
      fclose((*logs)[1].log_file);
      (*logs)[1].log_file = NULL;
#ifdef DEBUG_OUTPUT_TO_SCREEN
      printf("Closed\n");
#endif
    }
#endif 
  }
#endif

  global_log_data.temp_src_filename = NULL;
  global_log_data.temp_src_linenumber = 0;
#endif /* DISABLE_LOGGER */
}

#ifndef DISABLE_LOGGER
void load_proccessname_from_proc()
{
  pid_t pid = getpid();
  char buffer[PATH_MAX];
  char procname[16];
  FILE * statfile;
  char *ptr;

  sprintf(buffer, "/proc/%d/stat", pid);
  statfile = fopen(buffer, "rt");
  fgets(buffer, PATH_MAX-1, statfile);
  fclose(statfile);

  ptr = (char *)strrchr(buffer, ')');
  *ptr = '\0';
  memset(procname, 0, sizeof procname);
  sscanf(buffer, "%d (%15c", &pid, procname);   /* comm[16] in kernel */

  set_processname(procname);
}

/* =========================================================================
    Internal function definitions
   ========================================================================= */

static char *get_command_name(char *commandpath)
{
  char *ptr;
#ifdef DEBUG_OUTPUT_TO_SCREEN
  printf("getting command name %s\n",commandpath);
#endif
  ptr = (char *)strrchr(commandpath, '/');
  if (ptr==NULL)
    ptr = commandpath;
  else
    ptr++;

#ifdef DEBUG_OUTPUT_TO_SCREEN
  printf("Concluded %s\n", ptr);
#endif
  return ptr;
}

void  workout_what_to_print(struct what_to_print_array *what_to_print, 
			    struct tag_log_file_data *log_struct)
{
  /* is this a syslog entry? */
  if (log_struct->log_filename[0]==0)
  {
    what_to_print->print_datetime = false;
    what_to_print->print_processname = false;
    what_to_print->print_pid = false;
  }
  else
  {
    what_to_print->print_datetime = true;
    what_to_print->print_processname = true;
 
    /* pid is dealt with at the syslog level if we're syslogging */
    what_to_print->print_pid = 
      (((log_struct->display_options & logoption_pid) == 0)?false:true);
  }

  what_to_print->print_srcfile = 
    (((log_struct->display_options & logoption_nfile) == 0)?true:false);
  what_to_print->print_srcline = 
    (((log_struct->display_options & logoption_nline) == 0)?true:false);
  
  what_to_print->print_errlevel = true;
  what_to_print->print_errtype = true;
}

void generate_message_details(char *message_details_buffer, 
                              int message_details_buffer_length,
                              struct tag_log_file_data *log_struct,
                              enum loglevels loglevel, enum logtypes logtype)
{
  char datebuffer[32];
  char processinfo[64];

  char *ptr = message_details_buffer;
  int   templen;
  int   len = message_details_buffer_length;

  char log_buffer[MAXLOGSIZE];
  const char *logtype_string;

  char loglevel_string[12]; /* max int size is 2 billion, or 10 digits */

  struct what_to_print_array what_to_print;

  workout_what_to_print(&what_to_print, log_struct);

#ifdef DEBUG_OUTPUT_TO_SCREEN
  printf("Making MessageDetails\n");
#endif

  *ptr = 0;
  /*
  datebuffer[0] = 0;
  ptr = datebuffer;
  */

  if (what_to_print.print_datetime)
  {
    time_t thetime;
    time(&thetime);

  /* some people might prefer localtime() to gmtime() */
    strftime(ptr, len, "%b %d %H:%M:%S", localtime(&thetime));
#ifdef DEBUG_OUTPUT_TO_SCREEN
    printf("date is %s\n", ptr);
#endif

    templen = strlen(ptr);
    len -= templen;
    if (what_to_print.print_processname || what_to_print.print_pid)
      strncat(ptr, " ", len);
    else
      strncat(ptr, ":", len);

    templen++;
    len --;
    ptr += templen;
  }

  /*
  processinfo[0] = 0;
  ptr = processinfo;
  */

  if (what_to_print.print_processname)
  {
    strncpy(ptr, global_log_data.processname, len);

    templen = strlen(ptr);
    len -= templen;
    ptr += templen;
  }

  if (what_to_print.print_pid)
  {
    pid_t pid = getpid();

    sprintf(ptr, "[%d]", pid);

    templen = strlen(ptr);
    len -= templen;
    ptr += templen;
  }

  if (what_to_print.print_srcfile || what_to_print.print_srcline)
  {
    char sprintf_buffer[8];
    char *buff_ptr;

    sprintf_buffer[0] = '[';
    if (what_to_print.print_srcfile)
    {
      strcpy(&sprintf_buffer[1], "%s");
      buff_ptr = &sprintf_buffer[3];
    }
    if (what_to_print.print_srcfile && what_to_print.print_srcline)
    {
      strcpy(&sprintf_buffer[3], ":");
      buff_ptr = &sprintf_buffer[4];
    }
    if (what_to_print.print_srcline)
    {
      strcpy(buff_ptr, "%d");
      buff_ptr = &buff_ptr[2];
    }
    strcpy(buff_ptr, "]");

    /* 
       ok sprintf string is ready, now is the 1st parameter src or linenumber
     */
    if (what_to_print.print_srcfile)
    {
      sprintf(ptr, sprintf_buffer, 
	      global_log_data.temp_src_filename, 
	      global_log_data.temp_src_linenumber);
    }
    else
    {
      sprintf(ptr, sprintf_buffer, global_log_data.temp_src_linenumber);
    }

#ifdef DEBUG_OUTPUT_TO_SCREEN
    printf("Process info is %s\n", ptr);
#endif

    templen = strlen(ptr);
    len -= templen;
    ptr += templen;

  }

  if (what_to_print.print_processname || what_to_print.print_pid ||
      what_to_print.print_srcfile || what_to_print.print_srcline)
  {
    strncat(ptr, ": ", len);
    len -= 2;
    ptr += 2;
  }

/*
  loglevel_string[0] = 0;
  ptr = loglevel_string;
*/

  if (what_to_print.print_errlevel)
  {
    if ((loglevel/10) >= (num_loglevel_chars-1))
    {
      sprintf(ptr, "%c%d", arr_loglevel_chars[num_loglevel_chars-1],
                                       loglevel/10);
    }
    else
    {
      sprintf(ptr, "%c", arr_loglevel_chars[loglevel/10]);
    }

    templen = strlen(ptr);
    len -= templen;
    ptr += templen;    
  }

  if (what_to_print.print_errtype)
  {
    const char *logtype_string;

    /* get string represnetation of the Log Type */
    if (logtype<num_logtype_strings)
      logtype_string = arr_logtype_strings[logtype];
    else
      logtype_string = "";

    if (what_to_print.print_errlevel)
    {
      strncat(ptr, ":", len);
      ptr++;
    }

    sprintf(ptr, "%s", logtype_string);
  }

  message_details_buffer[message_details_buffer_length-1] = 0;

#ifdef DEBUG_OUTPUT_TO_SCREEN
    printf("Message Details are %s\n", message_details_buffer);
#endif
}
#endif /* DISABLE_LOGGER */

int get_syslog_equivalent(enum loglevels loglevel)
{
  switch (loglevel/10)
  {
    /* The question is we know how bad it is for us,
                    but how should that translate in the syslogs?  */
    case 0: /* severe */
      return LOG_ERR;
    case 1: /* error */
      return LOG_ERR;
    case 2: /* warning */
      return LOG_WARNING;
    case 3: /* note */
      return LOG_NOTICE;
    case 4: /* information */
      return LOG_INFO;
    default: /* debug */
      return LOG_DEBUG;
  }
}

void setuplog(char *logsource, char *logtype, char *loglevel, char *filename)
{
#ifndef DISABLE_LOGGER 
  /* -setuplogtype <syslog|filelog> <logtype> <loglevel>*/
  /*
    This should be rewritten so that somehow logsource is assumed and everything
    can be taken from default if needs be.  
   */
  const char* sources[] = {"syslog", "filelog"};
  const char *null = "";
  int sourcenum, typenum, levelnum;
  log_file_data_pair *logs = log_file_arr[logtype_default];

  LOG(log_extradebug, logtype_logger, "Attempting setuplog: %s %s %s %s", 
      logsource, logtype, loglevel, filename);

  /*
   Do I need these?
  
  if (logsource==NULL)
    logsource=null;
  if (logtype==NULL)
    logtype=null;
  if (loglevel==NULL)
    loglevel=null;
  if (filename==NULL)
    filename=null;
  */

  if (logsource==NULL)
  {
    LOG(log_note, logtype_logger, "no logsource given");
  }
  else
  {
    for(sourcenum=0;sourcenum<NUMOF(sources);sourcenum++)
    {
      if (strcasecmp(logsource, sources[sourcenum])==0)
        break;
    }
    if (sourcenum>=NUMOF(sources))
    {
      LOG(log_warning, logtype_logger, "%s is not a valid log source", logsource);
    }
    if ((sourcenum>0) && (filename==NULL))
    {
      LOG(log_warning, logtype_logger, 
          "when specifying a filelog, you must specify a valid filename");
    }
  }

  if (logtype==NULL)
  {
    LOG(log_note, logtype_logger, "no logsource given");
  }
  else
  {
    for(typenum=0;typenum<num_logtype_strings;typenum++)
    {
      if (strcasecmp(logtype, arr_logtype_strings[typenum])==0)
        break;
    }
    if (typenum>=num_logtype_strings)
    {
      LOG(log_warning, logtype_logger, "%s is not a valid log type", logtype);
    }
  }

  if (loglevel==NULL)
  {
    LOG(log_note, logtype_logger, "no logsource given");
  }
  else
  {
    for(levelnum=0;levelnum<num_loglevel_strings;levelnum++)
    {
      if (strcasecmp(loglevel, arr_loglevel_strings[levelnum])==0)
        break;
    }
    if (levelnum>=num_loglevel_strings)
    {
      LOG(log_warning, logtype_logger, "%s is not a valid log level", loglevel);
    }
  }

  /* check validity */
  if ((sourcenum>=NUMOF(sources)) || (typenum>=num_logtype_strings) ||
      (levelnum>=num_loglevel_strings))
    return;

  switch(sourcenum)
  {
  case 0: /* syslog */
    LOG(log_note, logtype_logger, "Doing syslog_setup(%d, %d, ...)", levelnum, typenum);
    syslog_setup(levelnum, typenum, 
		 (*logs)[0].display_options,
		 global_log_data.facility);
    break;
  default: /* filelog */
    LOG(log_note, logtype_logger, "Doing log_setup(%s, %d, %d, ...)", filename, levelnum, typenum);
    log_setup(filename, levelnum, typenum, 
	      (*logs)[0].display_options);
  };  
  return;
#endif /* DISABLE_LOGGER */
}







