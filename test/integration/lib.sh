# Shared helpers for the netatalk integration scripts.
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
# Sourced, not executed. Run as root inside the netatalk testsuite container.

fail() {
    fail_msg=$1
    echo "FAIL: $fail_msg" >&2
    exit 1
}
pass() {
    pass_msg=$1
    echo "ok: $pass_msg"
    return 0
}
# A precondition this image cannot meet: say so and end the whole script green.
skip_all() {
    skip_msg=$1
    echo "SKIP: $skip_msg"
    exit 0
}

# Negate a command, for use as a wait_for predicate.
not_() {
    if "$@"; then
        return 1
    fi

    return 0
}

# Poll a condition command until it succeeds or $1 seconds have elapsed.
# 0 as soon as it succeeds, 1 once the deadline passes. The bound is wall
# clock, not a tick count: a predicate that blocks (nc waits for a connect)
# must not stretch the wait past what the caller asked for.
wait_for() {
    wait_deadline=$(($(date +%s) + $1))
    shift

    while :; do
        if "$@" > /dev/null 2>&1; then
            return 0
        fi

        [ "$(date +%s)" -lt "$wait_deadline" ] || return 1
        sleep 0.1
    done
}

# Poll for the absence of a condition, same units.
wait_until_gone() {
    gone_secs=$1
    shift

    if wait_for "$gone_secs" not_ "$@"; then
        return 0
    fi

    return 1
}

# True while a netatalk controller or an afpd belonging to $1 (a username, or
# the empty string for any user) is running.
netatalk_processes() {
    np_user=$1

    if [ -n "$np_user" ]; then
        if pgrep -u "$np_user" netatalk || pgrep -u "$np_user" afpd; then
            return 0
        fi
    elif pgrep netatalk || pgrep afpd; then
        return 0
    fi

    return 1
}

# Stop every netatalk and afpd of user $1 (empty string = all) and wait for
# them to go away, so the next start cannot trip over a stale lock file or
# connect to the listener of the daemon that was just stopped.
stop_netatalk() {
    sn_user=$1

    if [ -n "$sn_user" ]; then
        pkill -u "$sn_user" netatalk 2> /dev/null || true
        pkill -u "$sn_user" afpd 2> /dev/null || true
    else
        pkill netatalk 2> /dev/null || true
        pkill afpd 2> /dev/null || true
    fi

    wait_until_gone 10 netatalk_processes "$sn_user" \
        || fail "netatalk or afpd of ${sn_user:-any user} did not stop"
    return 0
}

# True when something is accepting connections on port $1 (default 548) of
# host $2 (default 127.0.0.1).
# busybox nc has no working -z, so a one-shot connect with stdin closed is the
# portable probe: it exits 0 on an accepted connection and 1 on refused.
afp_listening() {
    al_port=${1:-548}
    al_host=${2:-127.0.0.1}

    if nc -w 1 "$al_host" "$al_port" < /dev/null > /dev/null 2>&1; then
        return 0
    fi

    return 1
}

# True when a controller startup line has been appended to log file $1 after
# its first $2 lines. The log is opened O_APPEND and never truncated, so a
# line left by an earlier start must not satisfy a later wait.
controller_started() {
    cs_log=$1
    cs_lines=$2

    if tail -n +"$((cs_lines + 1))" "$cs_log" 2> /dev/null \
        | grep -q "Netatalk AFP server starting"; then
        return 0
    fi

    return 1
}

# Start a root-service netatalk and wait until it accepts a connection and
# its controller has written its startup line to $LOG (default
# /var/log/afpd.log, the "log file" every root-service script configures).
# A daemonized controller that lost its logger looks alive from outside and
# is caught here rather than in a later grep. The controller parses the
# config and takes the lock file before it daemonizes, so a refusal is a
# non-zero exit of this foreground command; the || keeps the callers' set -e
# from ending the script without a FAIL: line.
start_root_netatalk() {
    srn_log=${LOG:-/var/log/afpd.log}
    srn_lines=0

    if [ -f "$srn_log" ]; then
        srn_lines=$(($(wc -l < "$srn_log") + 0))
    fi

    netatalk || fail "netatalk refused to start"
    wait_for 10 afp_listening \
        || fail "netatalk started but is not accepting connections on port 548"
    wait_for 10 controller_started "$srn_log" "$srn_lines" \
        || fail "controller startup line did not reach $srn_log"
    return 0
}

# Open a volume over SRP and enumerate its root: exit 0 when the volume
# opened, 2 when it could not be, 1 when authentication failed.
# $1 user, $2 password, $3 volume, $4 port (default 548). One spelling of the
# client invocation, so a change to the AFP version or the UAM flag is one
# edit. FPEnumerateExt drives AFP_ENUMERATE_EXT2, the request an AFP 3.x
# client makes, rather than the AFP 2 call the plain FPEnumerate verb sends.
afp_can_open() {
    aco_user=$1
    aco_pass=$2
    aco_vol=$3
    aco_port=${4:-548}
    afparg -7 -h 127.0.0.1 -p "$aco_port" -u "$aco_user" -w "$aco_pass" -A SRP \
        -s "$aco_vol" -f FPEnumerateExt > /dev/null 2>&1
    return $?
}

# Exit 0 when $1's owner and mode match $2, which is the single pair that
# stat prints, owner first: owned /srv/alice/.AppleDB "alice 700". A path that
# does not exist compares unequal and so is a failure.
owned() {
    owned_path=$1
    owned_want=$2

    if [ "$(stat -c '%U %a' "$owned_path" 2> /dev/null)" = "$owned_want" ]; then
        return 0
    fi

    return 1
}

# The value of one `afpd -v` line, by its label ($1, without the colon).
# Labels are right-aligned with leading spaces and tab-separated from the
# value, e.g. "                   afp.conf:<TAB>/etc/netatalk/afp.conf" or
# "              CNID backends:<TAB>dbd mysql sqlite ". Read from the binary
# rather than guessed: the testsuite image builds with a non-default prefix.
afpd_buildinfo() {
    ab_label=$1
    afpd -v | awk -F'\t' -v k="$ab_label:" 'index($1, k) {print $2; exit}'
    return $?
}

# The compiled-in configuration directory, where afppasswd's default stores
# live.
netatalk_confdir() {
    dirname "$(afpd_buildinfo afp.conf)"
    return $?
}

# True when afpd was built with the sqlite CNID backend.
have_sqlite_cnid() {
    if afpd_buildinfo 'CNID backends' | grep -qw sqlite; then
        return 0
    fi

    return 1
}
