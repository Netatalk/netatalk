#!/bin/sh
# Single-user mode integration tests.
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
have_sqlite_cnid || skip_all "sqlite CNID backend not built"

USER1=afpsingle
USER2=afpother
for u in $USER1 $USER2; do
    id $u > /dev/null 2>&1 || adduser -D $u
done
HOME1=$(getent passwd $USER1 | cut -d: -f6)
STATE=$HOME1/.local/state/netatalk
CONF=$HOME1/.config/netatalk
STORE=$CONF/afppasswd.srp
SHARE=$HOME1/Files
SHARE2=$HOME1/Backup
LOG1=$HOME1/netatalk.log
UUID1=550E8400-E29B-41D4-A716-446655440000
UUID2=6F9619FF-8B86-D011-B42D-00CF4FC964FF

trap 'stop_netatalk $USER1' EXIT
stop_netatalk $USER1

su $USER1 -c "mkdir -p -m 700 $CONF $STATE"
su $USER1 -c "mkdir -p $SHARE $SHARE2 $STATE/cnid"
su $USER1 -c "afppasswd -c -f -p $STORE -w Password1"
VERIFIER1=$STORE/$(id -u $USER1)
CNID1=$STATE/cnid/files
CNID_DIR_DIAG="requires the CNID directory of volume"

# Two volumes: "single-user" is about privileges and accounts, not about how
# many volumes the daemon may serve. The global vol dbpath puts each volume's
# CNID directory under the user's state directory, named after the volume, as
# the documented configuration does. Section names are lowercase because that
# is how iniparser stores them and how the served volume name reads.
write_conf() {
    su $USER1 -c "cat > $CONF/afp.conf" << EOF
[Global]
cnid scheme = sqlite
vol dbpath = $STATE/cnid
signature = singleuser-test
uam list = uams_srp.so
srp verifier path = $STORE
afp port = 5548
log file = $LOG1
zeroconf = no

[files]
path = $SHARE
volume uuid = $UUID1

[backup]
path = $SHARE2
volume uuid = $UUID2
EOF
    su $USER1 -c "chmod 600 $CONF/afp.conf"
}

# True once a start in the background has either bound port $1 or exited.
settled() {
    settled_port=$1
    settled_pid=$2

    if afp_listening "$settled_port"; then
        return 0
    fi

    if kill -0 "$settled_pid" 2> /dev/null; then
        return 1
    fi

    return 0
}

# Start with the current config and report whether afpd is accepting
# connections; stderr lands in /tmp/su_out. Every "must be rejected" assertion
# needs this rather than a grep alone, because a grep-only check passes when
# the diagnostic is printed as a warning and the daemon starts anyway.
try_start() {
    ts_port=${1:-5548}
    su $USER1 -c \
        "netatalk --single-user -P $STATE/netatalk.pid -F $CONF/afp.conf -d" \
        > /tmp/su_out 2>&1 &
    wait_for 10 settled "$ts_port" $! || return 1

    if afp_listening "$ts_port"; then
        return 0
    fi

    return 1
}

start_singleuser() {
    try_start || fail "the single-user daemon failed to start (see /tmp/su_out)"
}

# The owner's own AFP client. $@ = afparg arguments after the credentials.
# lib.sh's afp_can_open cannot be used here: a shell function does not cross
# su, and the owner's probes must run as the owner. The verb is the same one,
# FPEnumerateExt, for the reason lib.sh gives; afp_can_open serves the probes
# that need no identity switch.
owner_afparg() {
    su $USER1 -c \
        "afparg -7 -h 127.0.0.1 -p 5548 -u $USER1 -w Password1 -A SRP $*"
}

# --- an invalid config must be rejected BEFORE any load side effect. A volume
# with no uuid is skipped at load with the reason in the log, the config is
# refused for the skipped volumes, and afp_voluuid.conf is left alone.
VOLUUID_CONF=$(afpd_buildinfo afp_voluuid.conf)
[ -n "$VOLUUID_CONF" ] || fail "afpd -v does not report afp_voluuid.conf"
# The system state directory is lent to the test user so that a load which
# does generate a uuid can write it: without that the side effect this leg is
# about would be masked by a permission error.
mkdir -p "$(dirname "$VOLUUID_CONF")"
STATEDIR_OWNER=$(stat -c %U "$(dirname "$VOLUUID_CONF")")
chown $USER1 "$(dirname "$VOLUUID_CONF")"
rm -f "$VOLUUID_CONF"
write_conf
su $USER1 -c ": > $LOG1; sed -i '/volume uuid/d' $CONF/afp.conf"
try_start && fail "a config with no volume uuid was not rejected"
stop_netatalk $USER1
chown "$STATEDIR_OWNER" "$(dirname "$VOLUUID_CONF")"
[ ! -s "$VOLUUID_CONF" ] || fail "afp_voluuid.conf was mutated by an invalid config"
grep -q "requires every configured volume to load" /tmp/su_out \
    || fail "the refusal does not report the skipped volumes"
grep -q "volume \"files\": single-user mode requires an explicit 'volume uuid'" $LOG1 \
    || fail "the log does not name the volume without a uuid"
pass "an invalid config is rejected before any load side effect"

# --- the clause matrix: each bad config trips exactly its own clause
check_reject() {
    cr_case=$1
    cr_mutation=$2
    cr_diag=$3
    write_conf
    su $USER1 -c "$cr_mutation"
    try_start && {
        stop_netatalk $USER1
        fail "$cr_case: the daemon started instead of rejecting the config"
    }
    stop_netatalk $USER1
    grep -q "$cr_diag" /tmp/su_out \
        || fail "$cr_case: the expected diagnostic is missing"
    pass "$cr_case rejected"
    return 0
}

# --- one volume without a uuid refuses the whole config: the survivor would
# otherwise be served alone, with the skip known only to the log
check_reject "one volume without a uuid" \
    ": > $LOG1; sed -i '/volume uuid = $UUID2/d' $CONF/afp.conf" \
    "requires every configured volume to load"
grep -q "volume \"backup\": single-user mode requires an explicit 'volume uuid'" $LOG1 \
    || fail "the log does not name the one volume without a uuid"
grep -q "volume \"files\": single-user mode" $LOG1 \
    && fail "the volume with a uuid was reported as skipped"
check_reject "a volume section without a path" \
    ": > $LOG1; sed -i 's|^path = $SHARE2$|paht = $SHARE2|' $CONF/afp.conf" \
    "requires every configured volume to load"
grep -q "section \[backup\] has no 'path'" $LOG1 \
    || fail "the log does not name the section without a path"

check_reject "no uam list at all" \
    "sed -i '/uam list =/d' $CONF/afp.conf" \
    "requires 'uam list = uams_srp.so'"
check_reject "uam list not SRP-only" \
    "sed -i 's/uam list = uams_srp.so/uam list = uams_srp.so uams_dhx2.so/' $CONF/afp.conf" \
    "requires 'uam list = uams_srp.so'"
check_reject "config file not private" \
    "chmod 664 $CONF/afp.conf" \
    "not writable by group or others"
check_reject "config directory not private" \
    "chmod 755 $CONF" \
    "lives in a mode-0700 directory owned by that user"
su $USER1 -c "chmod 700 $CONF"
check_reject "CNID directory that cannot be created" \
    "sed -i 's|^vol dbpath = .*|vol dbpath = /var/lib/nowhere|' $CONF/afp.conf" \
    "$CNID_DIR_DIAG"
# a symbolic link or a read-only directory would fail at every mount instead,
# so both are refused here; the link is tried by a volume-level vol dbpath and
# under the [Global] one, which appends the volume name and a trailing slash
check_reject "CNID directory that is a symbolic link" \
    "ln -sfn $STATE/cnid/files $STATE/cnid/link; printf 'vol dbpath = %s/link\n' $STATE/cnid >> $CONF/afp.conf" \
    "$CNID_DIR_DIAG"
su $USER1 -c "rm $STATE/cnid/link"
check_reject "CNID directory that is a symbolic link under a [Global] vol dbpath" \
    "ln -sfn $STATE/cnid/files $STATE/cnid/backup" \
    "$CNID_DIR_DIAG"
su $USER1 -c "rm $STATE/cnid/backup"
check_reject "CNID directory that exists but is not writable" \
    "mkdir -m 500 $STATE/cnid/backup" \
    "$CNID_DIR_DIAG"
su $USER1 -c "rmdir $STATE/cnid/backup"
check_reject "no signature" \
    "sed -i '/^signature =/d' $CONF/afp.conf" \
    "requires an explicit .Global. signature"
check_reject "a .Homes. section" \
    "printf '\n[Homes]\nbasedir regex = /home\n' >> $CONF/afp.conf" \
    "does not support .Homes. volumes"
check_reject "a .Homes. section without a basedir regex" \
    "printf '\n[Homes]\npath = Public\n' >> $CONF/afp.conf" \
    "does not support .Homes. volumes"
# nested state would make each volume root its own CNID directory, which the
# owner-only rule would then close to everyone else
check_reject "vol dbnest" \
    "sed -i 's/^\[Global\]\$/[Global]\nvol dbnest = yes/' $CONF/afp.conf" \
    "does not support 'vol dbnest'"
check_reject "a non-sqlite CNID scheme" \
    "sed -i 's/cnid scheme = sqlite/cnid scheme = last/' $CONF/afp.conf" \
    "must use sqlite CNID"
check_reject "a volume the calling user cannot read" \
    "chmod 0 $SHARE2" \
    "cannot access volume"
su $USER1 -c "chmod 755 $SHARE2"
# Spotlight itself is allowed (the fixture leaves it at the default, on): the
# cnid backend searches the volume's own sqlite database. The other backends
# keep their index under system directories.
check_reject "a Spotlight backend other than cnid" \
    "printf 'spotlight backend = xapian\n' >> $CONF/afp.conf" \
    "supports only 'spotlight backend = cnid'"

# --- the SRP store is the verifier directory and the caller's file in it: the
# UAM opens both on every login, so a directory others can enter and a file
# others can read are each refused
check_reject "verifier directory not private" \
    "chmod 755 $STORE" \
    "requires 'srp verifier path' to be a mode-0700 directory"
su $USER1 -c "chmod 700 $STORE"
check_reject "verifier file not private" \
    "chmod 644 $VERIFIER1" \
    "requires 'srp verifier path' to be a mode-0700 directory"
su $USER1 -c "chmod 600 $VERIFIER1"
check_reject "verifier with a second hard link" \
    "ln $VERIFIER1 $STORE/second" \
    "requires 'srp verifier path' to be a mode-0700 directory"
su $USER1 -c "rm $STORE/second"

# --- a verifier the UAM serves is not refused: the file rule is "no group or
# other bits", not one exact mode
write_conf
su $USER1 -c "chmod 400 $VERIFIER1"
try_start || {
    su $USER1 -c "chmod 600 $VERIFIER1"
    fail "a read-only verifier with no group or other bits was refused"
}
stop_netatalk $USER1
su $USER1 -c "chmod 600 $VERIFIER1"
pass "a verifier with owner bits only is accepted"

# --- fixture for the refusal leg below: the store also holds a second
# account's verifier. Root's afppasswd -a requires a root-owned directory and
# a user's afppasswd -c writes only that user's own file, so the second
# verifier is built by $USER2 in a private directory of its own and copied
# into the owner's store with its owner and mode kept.
VER2=/tmp/su_v2
rm -rf $VER2
su $USER2 -c "afppasswd -c -p $VER2 -w Password2"
cp -p "$VER2/$(id -u $USER2)" $STORE/
rm -rf $VER2
VERIFIER2=$STORE/$(id -u $USER2)
owned $VERIFIER2 "$USER2 600" \
    || fail "the second verifier lost its owner or mode"
owned $STORE "$USER1 700" \
    || fail "the verifier directory lost its owner or mode"

# --- the documented bootstrap, started and used once: both volumes open as
# the owner
write_conf
start_singleuser
owner_afparg -s files -f FPEnumerateExt > /dev/null \
    || fail "single-user login and volume open as the owner failed"
owner_afparg -s backup -f FPEnumerateExt > /dev/null \
    || fail "the second single-user volume could not be opened"
# a process without root cannot change its group list and already has its
# user's, so a login must not try and log the refusal
grep -q "initgroups" $LOG1 \
    && fail "a single-user login tried to set the group list"

# --- a connection that opens a DSI session and drops before logging in must
# end its afpd child rather than wait out 'disconnect time'; the bytes are a
# DSIOpenSession request. The child's own last line is the observable, and only
# the listening afpd may remain: an exited child the parent has not reaped yet
# is a zombie, not a survivor, so state Z is not counted. The state comes from
# /proc, because busybox ps cuts the user column to eight characters.
only_afpd_parent() {
    oap_live=0

    for oap_pid in $(pgrep -u $USER1 afpd); do
        oap_state=$(awk '{print $3}' "/proc/$oap_pid/stat" 2> /dev/null)
        [ "$oap_state" = Z ] || oap_live=$((oap_live + 1))
    done

    [ "$oap_live" -le 1 ]
}
wait_for 10 only_afpd_parent \
    || fail "a child of an earlier login is still running"
drop_lines=$(($(wc -l < "$LOG1") + 0))
{
    printf '\000\004\000\001\000\000\000\000\000\000\000\000\000\000\000\000'
    sleep 1
} | timeout 3 nc 127.0.0.1 5548 > /dev/null 2>&1 || true
# only lines written after the drop count, as controller_started reads a log
dropped_child_terminated() {
    tail -n +"$((drop_lines + 1))" "$LOG1" \
        | grep -q "Disconnected session terminating"
}
wait_for 10 dropped_child_terminated \
    || {
        grep -iE "dsi_disconnect|afp_alarm|afp_over_dsi" $LOG1 | tail -6 >&2
        fail "the child of a client that dropped before login did not terminate"
    }
wait_for 10 only_afpd_parent \
    || {
        ps -o pid,stat,etime,args | grep "[a]fpd" >&2
        fail "an afpd child survived a client that dropped before login"
    }
pass "a connection dropped before login leaves no child behind"

# --- a second account cannot authenticate: its verifier is a real record (a
# hex salt, not the disabled placeholder) but it is $USER2's mode-0600 file,
# which a daemon running as $USER1 cannot open, so srp_lookup_verifier()
# refuses before any proof is exchanged and login()'s uid check is never
# reached. The refusal is the UAM's own log line naming that uid's file.
grep -q "^$USER2:[0-9A-Fa-f]" $VERIFIER2 \
    || fail "the store has no usable verifier for $USER2"
afp_can_open $USER2 Password2 files 5548 \
    && {
        stop_netatalk $USER1
        fail "a second account was served by the single-user daemon"
    }
grep -q "can't open verifier .*/$(id -u $USER2)" $LOG1 \
    || {
        stop_netatalk $USER1
        fail "the second account's verifier was not refused by the uam"
    }
pass "a second account's verifier is unreadable to the single-user daemon and its login is refused"

# --- the CNID state lives where vol dbpath puts it, owner-only: the
# directory 0700, the database, WAL and SHM files 0600, all the serving user's
owned $CNID1 "$USER1 700" \
    || fail "single-user CNID directory is not owner-owned 0700"
ls $CNID1/*.sqlite > /dev/null \
    || fail "no CNID database was created under vol dbpath"
for f in $CNID1/*.sqlite $CNID1/*.sqlite-wal $CNID1/*.sqlite-shm; do
    [ -e "$f" ] || continue
    owned "$f" "$USER1 600" || fail "$f is not owner-owned 0600"
done
stop_netatalk $USER1
pass "single-user CNID state is owner-only under vol dbpath"

# --- the serving user's own tools keep that state owner-only: nad opens the
# same directory without the server's flag and must not widen it
su $USER1 -c "nad -F $CONF/afp.conf ls $SHARE" > /dev/null \
    || fail "nad could not open the single-user volume as its user"
owned $CNID1 "$USER1 700" \
    || fail "nad widened the CNID directory to $(stat -c %a $CNID1)"
for f in $CNID1/*.sqlite; do
    owned "$f" "$USER1 600" || fail "nad widened $f to $(stat -c %a "$f")"
done
pass "the serving user's nad keeps the CNID state owner-only"

# --- state the user's nad creates before the server's first start is the
# shared kind any non-root tool creates, and the server's first open makes it
# owner-only
rm -rf $CNID1
su $USER1 -c "nad -F $CONF/afp.conf ls $SHARE" > /dev/null \
    || fail "nad could not create the CNID state as its user"
[ -d "$CNID1" ] \
    || fail "nad did not create the CNID state under vol dbpath"
[ "$(stat -c %a "$CNID1")" != 700 ] \
    || fail "nad created owner-only state for a server that has not started"
start_singleuser
owner_afparg -s files -f FPEnumerateExt > /dev/null \
    || {
        stop_netatalk $USER1
        fail "the volume could not be opened over state nad created"
    }
stop_netatalk $USER1
owned $CNID1 "$USER1 700" \
    || fail "the server did not make the state nad created owner-only"
pass "state created by the user's nad is made owner-only at the server's first open"

# --- foreign-owned CNID state is refused at the first mount, not at startup:
# the controller judges only that the directory is writable, the backend
# refuses the open when the session runs it, and the other volume is
# unaffected. The directory is left writable so the ownership check is what
# answers, not the controller's clause or the directory open.
chown -R root $CNID1
chmod 0777 $CNID1
su $USER1 -c ": > $LOG1"
start_singleuser
owner_afparg -s files -f FPEnumerateExt > /dev/null 2>&1 \
    && {
        stop_netatalk $USER1
        fail "a volume with CNID state the owner does not own was opened"
    }
owner_afparg -s backup -f FPEnumerateExt > /dev/null \
    || {
        stop_netatalk $USER1
        fail "the volume with usable state was not served"
    }
stop_netatalk $USER1
grep -q "not owned by the server user" $LOG1 \
    || fail "the backend's ownership diagnostic is missing from the log"
chown -R $USER1 $CNID1
pass "foreign-owned CNID state refuses the mount of that volume only"

# --- the AFP desktop database sits beside the CNID state, owned by the
# serving user, inside the 0700 directory that keeps it private. An AFP 2
# client would create it through FPOpenDT; afparg drives the same call.
su $USER1 -c ": > $LOG1"
start_singleuser
owner_afparg -s files -f FPOpenDT > /dev/null 2>&1 \
    || {
        stop_netatalk $USER1
        fail "FPOpenDT failed on a single-user volume"
    }
stop_netatalk $USER1
owned $CNID1 "$USER1 700" \
    || fail "the CNID directory is no longer owner-only after the desktop database was created"
[ "$(stat -c %U $CNID1/.AppleDesktop 2> /dev/null)" = "$USER1" ] \
    || fail "the desktop database is not owned by the serving user"
pass "the desktop database is created by the serving user under vol dbpath"

# --- a relative -F is resolved before afpd is handed it. This start
# daemonizes, because daemonize() is what moves the process to /: with -d the
# controller keeps its working directory and afpd would parse the relative
# path from there whether or not it had been resolved.
cd $CONF
su $USER1 -c "netatalk --single-user -P $STATE/netatalk.pid -F afp.conf" \
    > /tmp/su_out 2>&1 \
    || {
        cd /
        fail "a relative -F was refused (see /tmp/su_out)"
    }
cd /
wait_for 10 afp_listening 5548 \
    || {
        stop_netatalk $USER1
        fail "a relative -F did not start afpd (see /tmp/su_out and $LOG1)"
    }
stop_netatalk $USER1
pass "a relative -F is resolved before afpd is started"

# --- a second start against a running instance is refused by the lock file
# before the configuration is parsed: the second start is given a config that
# fails validation, so only the lock can be what refused it
start_singleuser
su $USER1 -c "sed -i '/uam list =/d' $CONF/afp.conf"
su $USER1 -c "netatalk --single-user -P $STATE/netatalk.pid -F $CONF/afp.conf -d" \
    > /tmp/su_out 2>&1 \
    && {
        stop_netatalk $USER1
        fail "a second start against a running instance was not refused"
    }
stop_netatalk $USER1
grep -qi "lock" /tmp/su_out \
    || fail "the refusal does not name the lock file"
grep -q "requires 'uam list" /tmp/su_out \
    && fail "the second start parsed its config before the lock file refused it"
pass "a start against a running instance is refused by the lock file before its config is parsed"

# --- a privileged port warns but does not stop the daemon. Whether a process
# without root can bind 548 here depends on the runtime's
# net.ipv4.ip_unprivileged_port_start, so the listener is not the observable;
# the controller is: the warning is printed at the end of a validation that
# passed, and the controller goes on to log its startup line, which a
# rejection never reaches.
write_conf
su $USER1 -c ": > $LOG1; sed -i 's/afp port = 5548/afp port = 548/' $CONF/afp.conf"
su $USER1 -c "netatalk --single-user -P $STATE/netatalk.pid -F $CONF/afp.conf -d" \
    > /tmp/su_out 2>&1 &
SU_PID=$!
wait_for 10 grep -q "is below 1024" /tmp/su_out \
    || fail "no warning for a privileged afp port"
wait_for 10 controller_started $LOG1 0 \
    || fail "the controller did not start after the warning (see /tmp/su_out and $LOG1)"
kill -0 $SU_PID 2> /dev/null \
    || fail "a privileged afp port was treated as a rejection"
stop_netatalk $USER1
pass "a privileged afp port warns without rejecting the config"

echo "ALL singleuser tests passed"
