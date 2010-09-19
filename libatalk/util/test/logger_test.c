#include <stdio.h>

#include <atalk/boolean.h>
#include <atalk/logger.h>

int main(int argc, char *argv[])
{
  set_processname("logger_Test");
#if 0
  LOG(log_severe, logtype_logger, "Logging Test starting: this should only log to syslog");

  /* syslog testing */
  LOG(log_severe, logtype_logger, "Disabling syslog logging.");
  unsetuplog("Default");
  LOG(log_error, logtype_default, "This shouldn't log to syslog: LOG(log_error, logtype_default).");
  LOG(log_error, logtype_logger, "This shouldn't log to syslog: LOG(log_error, logtype_logger).");
  setuplog("Default LOG_INFO");
  LOG(log_info, logtype_logger, "Set syslog logging to 'log_info', so this should log again. LOG(log_info, logtype_logger).");
  LOG(log_error, logtype_logger, "This should log to syslog: LOG(log_error, logtype_logger).");
  LOG(log_error, logtype_default, "This should log to syslog. LOG(log_error, logtype_default).");
  LOG(log_debug, logtype_logger, "This shouldn't log to syslog. LOG(log_debug, logtype_logger).");
  LOG(log_debug, logtype_default, "This shouldn't log to syslog. LOG(log_debug, logtype_default).");
  LOG(log_severe, logtype_logger, "Disabling syslog logging.");
  unsetuplog("Default");
#endif
  /* filelog testing */

  setuplog("DSI log_maxdebug test.log");
  LOG(log_info, logtype_dsi, "This should log.");
  LOG(log_error, logtype_default, "This should not log.");

  setuplog("Default log_debug test.log");
  LOG(log_debug, logtype_default, "This should log.");
  LOG(log_maxdebug, logtype_default, "This should not log.");

  LOG(log_maxdebug, logtype_dsi, "This should still log.");

  /* flooding prevention check */
  LOG(log_debug, logtype_default, "Flooding 12x1");
  for (int i = 0; i < 12; i++) {
      LOG(log_error, logtype_default, "flooding 12x1 1");
  }

  LOG(log_debug, logtype_default, "=============");
  LOG(log_debug, logtype_default, "Flooding 12x3");
  for (int i = 0; i < 12; i++) {
      LOG(log_error, logtype_default, "flooding 12x3 1");
      LOG(log_error, logtype_default, "flooding 12x3 2");
      LOG(log_error, logtype_default, "flooding 12x3 3");
  }

  LOG(log_debug, logtype_default, "=============");
  LOG(log_debug, logtype_default, "Flooding 12x4");
  for (int i = 0; i < 12; i++) {
      LOG(log_error, logtype_default, "flooding 12x4 1");
      LOG(log_error, logtype_default, "flooding 12x4 2");
      LOG(log_error, logtype_default, "flooding 12x4 3");
      LOG(log_error, logtype_default, "flooding 12x4 4");
  }

  LOG(log_debug, logtype_default, "=============");
  LOG(log_debug, logtype_default, "Flooding 12x5");
  for (int i = 0; i < 12; i++) {
      LOG(log_error, logtype_default, "flooding 12x5 1");
      LOG(log_error, logtype_default, "flooding 12x5 2");
      LOG(log_error, logtype_default, "flooding 12x5 3");
      LOG(log_error, logtype_default, "flooding 12x5 4");
      LOG(log_error, logtype_default, "flooding 12x5 5");
  }

  return 0;
}


