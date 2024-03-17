/*
  Copyright (c) 2009 Frank Lahm <franklahm@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdbool.h>

#include <atalk/logger.h>

int main(int argc _U_, char *argv[] _U_)
{
  set_processname("logger_Test");

  /* syslog testing */

  setuplog("default:severe", NULL, false);
  LOG(log_severe, logtype_logger, "[1/4] Syslog Test starting: this should only log to syslog LOG(log_severe, logtype_default).");

  setuplog("default:info", NULL, false);
  LOG(log_info, logtype_logger, "[2/4] Set syslog logging to 'log_info', so this should log again. LOG(log_info, logtype_logger).");
  LOG(log_error, logtype_logger, "[3/4] This should log to syslog: LOG(log_error, logtype_logger).");
  LOG(log_error, logtype_default, "[4/4] This should log to syslog. LOG(log_error, logtype_default).");
  LOG(log_debug, logtype_logger, "[1/2] This shouldn't log to syslog. LOG(log_debug, logtype_logger).");
  LOG(log_debug, logtype_default, "[2/2] This shouldn't log to syslog. LOG(log_debug, logtype_default).");

  /* filelog testing */

  setuplog("default:severe", "test.log", true);
  LOG(log_severe, logtype_logger, "[1/4] Filelog Test starting: this should only log to file LOG(log_severe, logtype_default).");
  setuplog("dsi:maxdebug", "test.log", true);
  LOG(log_info, logtype_dsi, "[2/4] This should log LOG(log_info, logtype_dsi).");
  LOG(log_error, logtype_default, "[1/2] This should not log LOG(log_default, logtype_dsi).");

  setuplog("default:debug", "test.log", true);
  LOG(log_debug, logtype_default, "[3/4] This should log LOG(log_debug, logtype_default).");
  LOG(log_maxdebug, logtype_default, "[2/2] This should not log LOG(log_maxdebug, logtype_default).");

  LOG(log_maxdebug, logtype_dsi, "[4/4] This should still log LOG(log_maxdebug, logtype_dsi).");

  /* flooding prevention check */

  LOG(log_debug, logtype_default, "Flooding 3x");
  for (int i = 0; i < 3; i++) {
      LOG(log_debug, logtype_default, "Flooding...");
  }

  /* wipe the array */

  LOG(log_debug, logtype_default, "1"); LOG(log_debug, logtype_default, "2"); LOG(log_debug, logtype_default, "3");

  LOG(log_debug, logtype_default, "-============");
  LOG(log_debug, logtype_default, "Flooding 5x");
  for (int i = 0; i < 5; i++) {
      LOG(log_debug, logtype_default, "Flooding...");
  }
  LOG(log_debug, logtype_default, "1"); LOG(log_debug, logtype_default, "2"); LOG(log_debug, logtype_default, "3");

  LOG(log_debug, logtype_default, "o============");
  LOG(log_debug, logtype_default, "Flooding 2005x");
  for (int i = 0; i < 2005; i++) {
      LOG(log_debug, logtype_default, "Flooding...");
  }
  LOG(log_debug, logtype_default, "1"); LOG(log_debug, logtype_default, "2"); LOG(log_debug, logtype_default, "3");

  LOG(log_debug, logtype_default, "0============");
  LOG(log_debug, logtype_default, "Flooding 11x1");
  for (int i = 0; i < 11; i++) {
      LOG(log_error, logtype_default, "flooding 11x1 1");
  }

  LOG(log_debug, logtype_default, "1============");
  LOG(log_debug, logtype_default, "Flooding 11x2");
  for (int i = 0; i < 11; i++) {
      LOG(log_error, logtype_default, "flooding 11x2 1");
      LOG(log_error, logtype_default, "flooding 11x2 2");
  }

  LOG(log_debug, logtype_default, "2============");
  LOG(log_debug, logtype_default, "Flooding 11x3");
  for (int i = 0; i < 11; i++) {
      LOG(log_error, logtype_default, "flooding 11x3 1");
      LOG(log_error, logtype_default, "flooding 11x3 2");
      LOG(log_error, logtype_default, "flooding 11x3 3");
  }

  LOG(log_debug, logtype_default, "3============");
  LOG(log_debug, logtype_default, "Flooding 11x4");
  for (int i = 0; i < 11; i++) {
      LOG(log_error, logtype_default, "flooding 11x4 1");
      LOG(log_error, logtype_default, "flooding 11x4 2");
      LOG(log_error, logtype_default, "flooding 11x4 3");
      LOG(log_error, logtype_default, "flooding 11x4 4");
  }


  return 0;
}

