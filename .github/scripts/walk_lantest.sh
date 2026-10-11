#!/bin/sh
# Walk timing experiment (scratch branch walk-timing, never merged): the
# environment of vm_spectest.sh, then afp_lantest instead of afp_spectest.
set -e

export AFP_USER=atalk1
export AFP_USER2=atalk2
export AFP_GROUP=afpusers
AFP_PASS="$(openssl rand -base64 12 | tr -d '/+=' | cut -c1-8)"
export AFP_PASS
export AFP_PASS2="$AFP_PASS"
export SHARE_NAME=test1
export SHARE_NAME2=test2
export NETATALK_SHARE_DIR=/mnt/afpshare
export NETATALK_BACKUP_DIR=/mnt/afpbackup
export NETATALK_LOG_FILE=/var/log/afpd.log
export AFP_DIRCACHESIZE=1048576
export AFP_DIRCACHE_MODE=arc
export AFP_DIRCACHE_VALIDATION_FREQ=100
export AFP_DIRCACHE_RFORK_BUDGET=1048576000
export AFP_DIRCACHE_RFORK_MAXSIZE=1048576
export AFP_CNID_BACKEND=sqlite
export AFP_CONVERT_APPLEDOUBLE=no
export AFP_EXTMAP=1
export INSECURE_AUTH=1
export DISABLE_TIMEMACHINE=1
export SERVER_NAME=Netatalk
export AFP_HOST=127.0.0.1
export AFP_PORT=548
export AFP_VERSION=7
export TESTSUITE=lan
export VERBOSE=1

[ -f "$NETATALK_CONFDIR/extmap.conf" ] || cp config/extmap.conf "$NETATALK_CONFDIR/extmap.conf"

set +u
# shellcheck source=/dev/null
. ./distrib/docker/env_setup_netatalk.sh
set -u

# shellcheck disable=SC2086
$SERVICE_START
sleep 3

set +e
afp_lantest -7 -h "$AFP_HOST" -p "$AFP_PORT" -u "$AFP_USER" -w "$AFP_PASS" \
    -s "$SHARE_NAME" -n 1
LANTEST_RC=$?
set -e

# shellcheck disable=SC2086
$SERVICE_STOP 2> /dev/null || true
exit "$LANTEST_RC"
