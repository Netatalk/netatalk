#!/bin/sh
# Shared AFP spectest driver for the BSD/illumos/Solaris VM legs in
# .github/workflows/spectest-vms.yml. Runs inside the VM (vmactions shell)
# after the build/install step, from the synced repository root.
#
# Per-OS contract, exported by the calling workflow step:
#   NETATALK_CONFDIR  compiled-in config dir (e.g. /usr/local/etc)
#   NETATALK_LOCKDIR  compiled-in lockfile dir (see meson.build)
#   SERVICE_START     command that starts the packaged netatalk service
#   SERVICE_STOP      command that stops it
#   AFP_ADOUBLE       set to 1 when the share filesystem has no native EA
#                     support (ea = ad + afp_spectest -a); leave unset for
#                     the real ea = sys PerfConfig
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
# Production max-performance tuning knobs
export AFP_DIRCACHESIZE=1048576
export AFP_DIRCACHE_MODE=arc
export AFP_DIRCACHE_VALIDATION_FREQ=100
export AFP_DIRCACHE_RFORK_BUDGET=1048576000
export AFP_DIRCACHE_RFORK_MAXSIZE=1048576
export AFP_CNID_BACKEND=dbd
export AFP_CONVERT_APPLEDOUBLE=no
export AFP_EXTMAP=1
export INSECURE_AUTH=1
export DISABLE_TIMEMACHINE=1
export SERVER_NAME=Netatalk
export AFP_HOST=127.0.0.1
export AFP_PORT=548
export AFP_VERSION=7
export TESTSUITE=spectest
export VERBOSE=1

# copy the extmap template if absent so env_setup can activate it
[ -f "$NETATALK_CONFDIR/extmap.conf" ] || cp config/extmap.conf "$NETATALK_CONFDIR/extmap.conf"

# env_setup expects the container contract (no nounset); disable it
set +u
# shellcheck source=/dev/null  # sibling repo file, not resolvable standalone
. ./distrib/docker/env_setup_netatalk.sh
set -u

echo "==== Starting netatalk (production config) ===="
# SERVICE_START holds a command with arguments; intentional word splitting.
# shellcheck disable=SC2086
$SERVICE_START
sleep 3
asip-status "$AFP_HOST:$AFP_PORT" || true

echo "==== Running AFP spec test (AFP 3.4) ===="
set +e
# TEST_FLAGS holds multiple flags (e.g. "-c /path" or "-a"); must split.
# shellcheck disable=SC2086
afp_spectest $TEST_FLAGS \
    -"$AFP_VERSION" \
    -h "$AFP_HOST" \
    -p "$AFP_PORT" \
    -u "$AFP_USER" \
    -d "$AFP_USER2" \
    -w "$AFP_PASS" \
    -s "$SHARE_NAME" \
    -S "$SHARE_NAME2"
SPECTEST_RC=$?
set -e

echo "==== afpd.log (tail) ===="
tail -200 "$NETATALK_LOG_FILE" 2> /dev/null || echo "no log file"
echo "==== afp.conf (redacted) ===="
sed -E 's/^([[:space:]]*[^=#]*(pass|pw|secret|key)[^=]*=).*/\1 ***REDACTED***/I' \
    "$NETATALK_CONFDIR/afp.conf" 2> /dev/null || true

# shellcheck disable=SC2086  # SERVICE_STOP is a command + args; must split
$SERVICE_STOP 2> /dev/null || true
exit "$SPECTEST_RC"
