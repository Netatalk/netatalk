#include <stdio.h>

#include <atalk/boolean.h>
#include <atalk/logger.h>

int main(int argc, char *argv[])
{
  bool retval;

  set_processname("logger_Test");

  LOG(log_severe, logtype_logger, "Logging Test starting: this should only log to syslog");

  /* syslog testing */
  LOG(log_severe, logtype_logger, "Disabling syslog logging.");
  setuplog("Default",  NULL, NULL);
  LOG(log_error, logtype_default, "This shouldn't log to syslog: LOG(log_error, logtype_default).");
  LOG(log_error, logtype_logger, "This shouldn't log to syslog: LOG(log_error, logtype_logger).");
  setuplog("Default",  "LOG_INFO", NULL);
  LOG(log_info, logtype_logger, "Set syslog logging to 'log_info', so this should log again. LOG(log_info, logtype_logger).");
  LOG(log_error, logtype_logger, "This should log to syslog: LOG(log_error, logtype_logger).");
  LOG(log_error, logtype_default, "This should log to syslog. LOG(log_error, logtype_default).");
  LOG(log_debug, logtype_logger, "This shouldn't log to syslog. LOG(log_debug, logtype_logger).");
  LOG(log_debug, logtype_default, "This shouldn't log to syslog. LOG(log_debug, logtype_default).");
  LOG(log_severe, logtype_logger, "Disabling syslog logging.");
  setuplog("Default",  NULL, NULL);

  /* filelog testing */
  setuplog("Default",  "LOG_INFO", "test.log");
  LOG(log_info, logtype_logger, "This should log.");
  LOG(log_info, logtype_default, "This should log.");
  LOG(log_error, logtype_logger, "This should log.");
  LOG(log_error, logtype_default, "This should log.");
  LOG(log_debug, logtype_logger, "This should not log.");
  LOG(log_debug, logtype_default, "This should not log.");

  LOG(log_severe, logtype_logger, "Logging Test finishing");

  return 0;
}


