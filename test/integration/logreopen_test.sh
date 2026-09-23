#!/bin/sh
# The controller's own log lines reach "log file" after it daemonizes.
# Run as root inside the netatalk testsuite container.
set -e
. /integration/lib.sh
trap 'stop_netatalk ""' EXIT
stop_netatalk ""

LOG=/var/log/afpd.log
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
[ "$(grep -c 'Netatalk AFP server starting' "$LOG")" -eq 2 ] \
    || fail "expected two controller startup lines in $LOG, found $(grep -c 'Netatalk AFP server starting' "$LOG")"
pass "a restart appends a second controller startup line"

echo "ALL logreopen tests passed"
