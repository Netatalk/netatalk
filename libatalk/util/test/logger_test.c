
#include <stdio.h>
#include <atalk/logger.h>

#include <atalk/boolean.h>
#include <atalk/logger.h>

int main(int argc, char *argv[])
{
  bool retval;

  /* LoadProccessNameFromProc(); */
  set_processname("logger_Test");
  log_init();

  LOG(log_severe, logtype_logger, "Logging Test starting");
  printf("Logging Test Starting\n");

  LOG(log_debug, logtype_default, logtype_logger, "Testing 123");
  LOG(log_error, logtype_logger, "Testing 456");

  retval = log_setup("/var/log/newlog.log", log_error, logtype_default, logoption_pid);
  retval = log_setup(NULL, log_error, logtype_logger, logoption_pid);

  LOG(log_debug, logtype_default, logtype_logger, "This shouldn't log");
  LOG(log_error, logtype_logger, "This should log");

  syslog_setup(log_error, logtype_default, logoption_pid, logfacility_user);

  LOG(log_debug, logtype_default, logtype_logger, "This shouldn't log");
  LOG(log_error, logtype_logger, "This should log");

  printf("Logging Test finishing\n");
  LOG(log_severe, logtype_logger, "Logging Test finishing");

  log_close();
  return 0;
}


