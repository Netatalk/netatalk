#!/bin/sh
# The controller's own log lines reach their destination after it daemonizes.
# Copyright (C) 2026  Andy Lemin (andylemin)
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# Run as root inside the netatalk testsuite container.
set -e
. /integration/lib.sh
trap 'stop_netatalk ""; pkill syslogd 2> /dev/null || true' EXIT
stop_netatalk ""

LOG=/var/log/afpd.log
STARTED="Netatalk AFP server starting"
CONF=$(netatalk_confdir)
mkdir -p "$CONF" /var/log
printf '[Global]\nlog file = %s\n' "$LOG" > "$CONF/afp.conf"
: > "$LOG"

# --- 1: a daemonized start writes the controller's startup line
start_root_netatalk
pass "controller startup line reached $LOG after daemonize"

# --- 2: a restart appends a second one; the wait in start_root_netatalk
# reads only the lines added since it was called, so the first line cannot
# satisfy it
stop_netatalk ""
start_root_netatalk
[ "$(grep -c "$STARTED" "$LOG")" -eq 2 ] \
    || fail "expected two controller startup lines in $LOG, found $(grep -c "$STARTED" "$LOG")"
pass "a restart appends a second controller startup line"

# --- 3: with no "log file" the controller logs to syslog. "log level = info"
# makes the logger announce itself during the config parse, so the syslog
# connection exists before the daemonize that closes every descriptor, and
# the startup line must still arrive.
if command -v syslogd > /dev/null; then
    stop_netatalk ""
    printf '[Global]\nlog level = default:info\n' > "$CONF/afp.conf"
    : > /var/log/messages
    syslogd -O /var/log/messages
    netatalk || fail "netatalk refused to start with syslog logging"
    wait_for 10 grep -q "$STARTED" /var/log/messages \
        || fail "controller startup line did not reach syslog after daemonize"
    stop_netatalk ""
    pkill syslogd || true
    pass "controller startup line reached syslog after daemonize"
fi

# --- 4: a "log file" in mkstemp form is written under its generated name
stop_netatalk ""
rm -f /tmp/netatalk.??????
printf '[Global]\nlog file = /tmp/netatalk.XXXXXX\n' > "$CONF/afp.conf"
netatalk || fail "netatalk refused to start with a mkstemp log file"
mkstemp_logged() {
    grep -lq "$STARTED" /tmp/netatalk.?????? 2> /dev/null
}
wait_for 10 mkstemp_logged \
    || fail "controller startup line did not reach the mkstemp log file"
stop_netatalk ""
pass "a mkstemp log file is written after daemonize"

# --- 5: a relative "log file" names the file in the controller's launch
# directory, and daemonize() chdir()s to / before the reopen, so the stored
# name must be absolute for the controller's startup line to reach that same
# file. afpd, started from / after the daemonize, resolves the same relative
# name from / and is not what this leg is about.
stop_netatalk ""
rm -f /var/log/afpd-rel.log /afpd-rel.log
printf '[Global]\nlog file = afpd-rel.log\n' > "$CONF/afp.conf"
cd /var/log
netatalk || {
    cd /
    fail "netatalk refused to start with a relative log file"
}
cd /
wait_for 10 grep -q "$STARTED" /var/log/afpd-rel.log \
    || fail "controller startup line did not reach the relative log file in the launch directory"
stop_netatalk ""
rm -f /afpd-rel.log
pass "a relative log file keeps its launch-directory location after daemonize"

echo "ALL logreopen tests passed"
