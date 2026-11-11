#!/bin/sh
#
# Collect diagnostics of a slow AFP server for its developers: a capture of
# the AFP traffic, the server's counters before and after, optionally a
# trace of one afpd session, and reports with every name and address taken
# out, packed in one file to send.
#
# Copyright (c) 2026 Andy Lemin (andylemin)
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
# usage: netatalk-diag [-c client] [-u user] [-p pid] [-i interface]
#                      [-o folder] [-d seconds] [-M MiB] [-t [-s]] [-m]
#                      [-b entries] [-l] [-F]
#   -c  capture and trace only the client at this address
#   -u  trace the session of this user
#   -p  trace the afpd session process with this pid
#   -i  capture on this interface (default: the one the client or the
#       default route uses)
#   -o  the folder to write, which must not exist (default:
#       /var/tmp/netatalk-diag-<date>)
#   -d  stop after this many seconds (default: when Enter is pressed)
#   -M  stop the capture at this many MiB (default: 1024, or a quarter of
#       the free space if that is less)
#   -t  trace the session: DTrace on FreeBSD, bpftrace on Linux
#   -s  with -t, count the session's system calls in place of its requests
#   -m  log the session at maxdebug and dump its dircache, for the run
#   -b  then time a read-only walk of at most this many entries per volume
#   -l  start before the client connects, to time its login
#   -F  package the reports even when the leak check finds a match
#
# Run as root. <folder>/netatalk-diagnostics-<date>.tar.gz is the file to
# send; the raw capture, the afpd log and the dircache dumps stay in
# <folder>.

set -u

# root runs the helper: no Perl code or option from the environment
unset PERL5LIB PERLLIB PERL5OPT PERLIO PERL_UNICODE

helper=@libexecdir@/netatalk/netatalk_diag.pl
perl=@PERL@
here=$(cd "$(dirname "$0")" && pwd)
# run from the source tree, the helper sits beside the script
[ -r "$helper" ] || helper=$here/netatalk_diag.pl
[ -x "$perl" ] || perl=perl

client=
user=
pid=
iface=
stamp=$(date +%Y%m%d-%H%M%S)
out=/var/tmp/netatalk-diag-$stamp
reports=netatalk-diagnostics-$stamp
duration=0
maxmib=
trace=0
syscalls=0
debug=0
entries=0
login=0
force=

usage() {
    sed -n '/^# usage:/,/^#   -F/s/^# \{0,1\}//p' "$0" >&2
    exit 2
}

die() {
    echo "netatalk-diag: $*" >&2
    exit 1
}

note() {
    echo "$*" >> "$out/notes.txt"
}

while getopts c:u:p:i:o:d:M:tsmb:lF opt; do
    case $opt in
        c) client=$OPTARG ;;
        u) user=$OPTARG ;;
        p) pid=$OPTARG ;;
        i) iface=$OPTARG ;;
        o) out=$OPTARG ;;
        d) duration=$OPTARG ;;
        M) maxmib=$OPTARG ;;
        t) trace=1 ;;
        s) syscalls=1 ;;
        m) debug=1 ;;
        b) entries=$OPTARG ;;
        l) login=1 ;;
        F) force=--force ;;
        *) usage ;;
    esac
done
[ "$OPTIND" -gt $# ] || usage

for n in "$duration" "$entries" ${maxmib:+"$maxmib"} ${pid:+"$pid"}; do
    case $n in
        '' | *[!0-9]*) usage ;;
        *) ;;
    esac
done

# sh arithmetic reads a number with a leading zero as octal
decimal() {
    n=${1#"${1%%[1-9]*}"}
    echo "${n:-0}"
}
duration=$(decimal "$duration")
entries=$(decimal "$entries")
[ -z "$maxmib" ] || maxmib=$(decimal "$maxmib")
[ -z "$pid" ] || pid=$(decimal "$pid")
[ "$syscalls" -eq 0 ] || [ "$trace" -eq 1 ] || die "-s goes with -t"
if [ "$login" -eq 1 ] && { [ "$trace" -eq 1 ] || [ "$debug" -eq 1 ]; }; then
    die "-l waits for a session that does not exist yet: no -t or -m"
fi
[ "$(id -u)" -eq 0 ] || die "run as root"
command -v "$perl" > /dev/null 2>&1 || die "perl is needed"
[ -r "$helper" ] || die "cannot read $helper"
command -v tcpdump > /dev/null 2>&1 || die "tcpdump is needed"
if [ "$duration" -eq 0 ] && [ ! -t 0 ]; then
    die "without a terminal, give -d with the seconds to capture"
fi
[ "$debug" -eq 0 ] || [ -t 0 ] || die "-m asks for steps on the Mac: run" \
    "it from a terminal"

os=$(uname -s)

# the raw capture, log and dumps hold names: root alone reads them, in a
# folder this run makes itself, so no other user placed anything in it.
# Nor may another user rename that folder or one above it: each above is
# root's, and only root writes in it or it is sticky, as /var/tmp is. The
# check starts at the deepest folder that exists, by its physical path,
# and the run makes each folder below it with mkdir alone, so a folder
# that another user makes there after the check stops the run.
umask 077
rest=$(basename "$out")
d=$(dirname "$out")
while [ ! -d "$d" ]; do
    rest=$(basename "$d")/$rest
    d=$(dirname "$d")
done
d=$(cd -P "$d" && pwd) || exit 1
out=${d%/}
while :; do
    if [ -z "$(find "$d" -prune -user 0 \( -perm -1000 -o ! -perm -020 \
        ! -perm -002 \) -print)" ]; then
        die "another user can change $d: give -o a folder below folders" \
            "that only root can change"
    fi
    [ "$d" != / ] || break
    d=$(dirname "$d")
done
ifs=$IFS
IFS=/
set -f
# shellcheck disable=SC2086
set -- $rest
set +f
IFS=$ifs
for part; do
    out=$out/$part
    shift
    mkdir "$out" 2> /dev/null && continue
    [ "$#" -gt 0 ] || die "$out exists; give -o a folder that does not"
    die "cannot make $out: give -o a folder below folders that only root" \
        "can change"
done
echo "netatalk-diag: writing to $out"

free=$(df -Pk "$out" | awk 'NR == 2 {print int($4 / 1024)}')
[ "${free:-0}" -ge 100 ] || die "only ${free:-0} MiB free in $out"
if [ -z "$maxmib" ]; then
    maxmib=$((free / 4))
    [ "$maxmib" -le 1024 ] || maxmib=1024
fi

helper_run() {
    "$perl" "$helper" "$@"
}

# ---- the sessions: pid, user and client of each connected afpd ----

list_sessions() {
    case $os in
        FreeBSD)
            sockstat -46 -c -w -P tcp -p 548 2> /dev/null \
                | awk '$2 == "afpd" {
                    c = $7
                    sub(/:[0-9]+$/, "", c)
                    gsub(/[][]/, "", c)
                    print $3, $1, c
                }'
            ;;
        Linux)
            ss -Htnp state established '( sport = :548 )' 2> /dev/null \
                | awk '{
                    c = $4
                    sub(/:[0-9]+$/, "", c)
                    gsub(/[][]/, "", c)
                    sub(/^::ffff:/, "", c)
                    if (match($0, /"afpd",pid=[0-9]+/)) {
                        p = substr($0, RSTART, RLENGTH)
                        sub(/.*pid=/, "", p)
                        print p, c
                    }
                }' | while read -r p c; do
                # procps shows a user name past its column's 8 characters
                # cut short, or as the uid
                echo "$p $(ps -o user:64= -p "$p" 2> /dev/null) $c"
            done
            ;;
        *) ;;
    esac
}

lookup=$((1 - login))
case $os in
    FreeBSD | Linux) ;;
    *)
        [ "$trace" -eq 0 ] && [ "$debug" -eq 0 ] \
            || die "-t and -m need FreeBSD or Linux"
        lookup=0
        note "no session lookup on $os: the capture alone"
        ;;
esac
if [ "$lookup" -eq 1 ]; then
    list_sessions | awk -v u="$user" -v c="$client" -v p="$pid" \
        '(u == "" || $2 == u) && (c == "" || $3 == c) \
         && (p == "" || $1 == p) && NF == 3' > "$out/sessions.all"
    n=$(wc -l < "$out/sessions.all" | tr -d ' ')
    if [ "$n" -eq 0 ]; then
        [ -z "$pid$user$client" ] || die "no afpd session matches;" \
            "connect the Mac to the volume first"
        die "no afpd session is connected; connect the Mac first, or give" \
            "-l to start before it connects"
    fi
    if [ "$n" -eq 1 ]; then
        pid=$(awk '{print $1}' "$out/sessions.all")
    else
        pid=
        [ "$trace" -eq 0 ] && [ "$debug" -eq 0 ] \
            || die "$n sessions match; give -p, -u or -c to pick one"
        note "$n sessions: the capture covers them all and none was traced"
    fi
    awk -v p="$pid" '{print $1, $2, $3, ($1 == p ? 1 : 0)}' \
        "$out/sessions.all" > "$out/sessions.txt"
    rm -f "$out/sessions.all"
    [ -z "$pid" ] || echo "netatalk-diag: the afpd session is pid $pid"
fi

# the user the session runs as, whose files alone -m reads
uid=
if [ "$debug" -eq 1 ]; then
    uid=$(ps -o uid= -p "$pid" 2> /dev/null | tr -d ' ')
    [ -n "$uid" ] || die "cannot read the user of session $pid"
fi

# ---- what the run must not leak, and the configuration ----

{
    echo "host $(hostname)"
    echo "host $(uname -n)"
    [ -z "$client" ] || echo "client $client"
    awk '{print "user " $2; print "client " $3}' "$out/sessions.txt" \
        2> /dev/null
    case $os in
        FreeBSD) ps -ax -o user=,comm= ;;
        Linux) ps -e -o user:64=,comm= ;;
        *) ps -e -o user=,comm= ;;
    esac 2> /dev/null | awk '$2 == "afpd" {print "user " $1}' | sort -u
    case $os in
        Linux)
            ip -o addr show 2> /dev/null \
                | awk '{a = $4; sub(/\/.*/, "", a); print "address " a}'
            ip -o link show 2> /dev/null | awk '{
                for (i = 1; i < NF; i++)
                    if ($i == "link/ether") print "mac " $(i + 1)
            }'
            ;;
        *)
            ifconfig -a 2> /dev/null | awk '
                $1 == "inet" || $1 == "inet6" {
                    a = $2
                    sub(/%.*/, "", a)
                    print "address " a
                }
                $1 == "ether" {print "mac " $2}'
            ;;
    esac
} > "$out/identity.txt"

# the route to the client picks the interface: -c, the traced session's
# client, or the one client of every session; several clients on Linux
# capture on every interface
to=$client
if [ -z "$to" ] && [ -s "$out/sessions.txt" ]; then
    to=$(awk -v p="$pid" '$1 == p {print $3}' "$out/sessions.txt")
    if [ -z "$to" ]; then
        to=$(awk '{print $3}' "$out/sessions.txt" | sort -u)
        if [ "$(echo "$to" | wc -l)" -gt 1 ]; then
            to=
            [ "$os" != Linux ] || [ -n "$iface" ] || iface=any
        fi
    fi
fi
if [ -z "$iface" ]; then
    case $os in
        Linux)
            iface=$(ip route get "${to:-1.1.1.1}" 2> /dev/null \
                | awk '{for (i = 1; i < NF; i++) if ($i == "dev") {
                    print $(i + 1); exit}}')
            ;;
        *)
            iface=$(route -n get "${to:-default}" 2> /dev/null \
                | awk '/interface:/ {print $2}')
            ;;
    esac
fi
[ -n "$iface" ] || die "no interface found; give -i"
echo "$iface" > "$out/iface.txt"

{
    uname -srm
    case $os in
        FreeBSD)
            freebsd-version -ku
            sysctl hw.model hw.ncpu hw.physmem kern.maxvnodes \
                vfs.zfs.arc.max vfs.zfs.arc_max 2> /dev/null
            ;;
        Linux)
            awk -F= '$1 == "PRETTY_NAME" {print $2}' /etc/os-release \
                2> /dev/null
            awk -F': ' '/^model name/ {print "cpu: " $2; exit}' /proc/cpuinfo
            echo "cpus: $(grep -c '^processor' /proc/cpuinfo)"
            awk '/^MemTotal/ {print "memory: " $2 " kB"}' /proc/meminfo
            ;;
        *) sysctl hw.model hw.ncpu hw.physmem 2> /dev/null ;;
    esac
} > "$out/system.txt" 2>&1

afpd -V > "$out/afpd-V.txt" 2>&1
conf=$(awk -F':[[:space:]]*' '/afp.conf:/ {print $2; exit}' \
    "$out/afpd-V.txt")
for f in "$conf" /usr/local/etc/afp.conf /etc/netatalk/afp.conf \
    /usr/local/etc/netatalk/afp.conf; do
    if [ -n "$f" ] && [ -r "$f" ]; then
        cp "$f" "$out/afp.conf.raw"
        break
    fi
done
[ -r "$out/afp.conf.raw" ] || note "no afp.conf found"

case $os in
    Linux) cp /proc/self/mountinfo "$out/mounts.txt" ;;
    *) mount -p > "$out/mounts.txt" 2> /dev/null ;;
esac

# a ZFS dataset under a volume gets its properties and its pool's layout
k=0
if [ -r "$out/afp.conf.raw" ] && command -v zfs > /dev/null 2>&1; then
    helper_run volumes "$out/afp.conf.raw" "$out/mounts.txt" \
        | awk -F '\t' '$2 == "zfs" {print $3}' | sort -u > "$out/datasets.raw"
    while read -r ds; do
        k=$((k + 1))
        echo "$k $ds" >> "$out/datasets.txt"
        zfs get all "$ds" > "$out/zfs-$k.txt" 2>&1
        zpool status "${ds%%/*}" > "$out/zpool-status-$k.txt" 2>&1
        zpool iostat -v "${ds%%/*}" > "$out/zpool-iostat-$k-before.txt" 2>&1
    done < "$out/datasets.raw"
    rm -f "$out/datasets.raw"
fi

{
    echo "interface $iface"
    case $os in
        Linux)
            ip -d link show dev "$iface" 2> /dev/null \
                | grep -v -e 'link/' -e 'alias'
            ethtool "$iface" 2> /dev/null \
                | grep -E 'Speed|Duplex|Link detected|Port:'
            sysctl net.ipv4.tcp_rmem net.ipv4.tcp_wmem net.core.rmem_max \
                net.core.wmem_max net.ipv4.tcp_congestion_control \
                net.ipv4.tcp_window_scaling net.ipv4.tcp_sack \
                net.ipv4.tcp_timestamps net.ipv4.tcp_slow_start_after_idle \
                net.core.default_qdisc 2> /dev/null
            ;;
        *)
            ifconfig "$iface" 2> /dev/null \
                | grep -E 'flags=|options=|media:|status:|mtu'
            sysctl net.inet.tcp.sendbuf_auto net.inet.tcp.sendbuf_max \
                net.inet.tcp.sendbuf_inc net.inet.tcp.recvbuf_auto \
                net.inet.tcp.recvbuf_max net.inet.tcp.sendspace \
                net.inet.tcp.recvspace net.inet.tcp.cc.algorithm \
                net.inet.tcp.delayed_ack net.inet.tcp.delacktime \
                net.inet.tcp.mssdflt net.inet.tcp.rfc1323 \
                net.inet.tcp.sack.enable net.inet.tcp.tso 2> /dev/null
            ;;
    esac
} > "$out/network.txt" 2>&1

# ---- counters, before and after ----

linux_snapshot() {
    for f in snmp netstat; do
        echo "== $f"
        cat "/proc/net/$f"
    done
    echo "== vmstat"
    cat /proc/vmstat
    echo "== diskstats"
    cat /proc/diskstats
    if [ -r /proc/spl/kstat/zfs/arcstats ]; then
        echo "== arcstats"
        cat /proc/spl/kstat/zfs/arcstats
    fi
    for f in cpu io memory; do
        if [ -r "/proc/pressure/$f" ]; then
            echo "== pressure.$f"
            cat "/proc/pressure/$f"
        fi
    done
    echo "== if"
    for f in /sys/class/net/"$iface"/statistics/*; do
        [ -r "$f" ] && echo "$(basename "$f"): $(cat "$f")"
    done
    if [ -n "$pid" ]; then
        for f in stat status io; do
            echo "== pid.$f"
            cat "/proc/$pid/$f"
        done
        echo "== pid.fds"
        find "/proc/$pid/fd" -mindepth 1 -maxdepth 1 | wc -l
    fi
}

snapshot() {
    tag=$1
    case $os in
        FreeBSD)
            sysctl vfs kern.maxvnodes kstat.zfs.misc.arcstats \
                > "$out/counters-$tag.txt" 2>&1
            vmstat -z > "$out/vmstat-z-$tag.txt" 2>&1
            netstat -s -p tcp > "$out/netstat-tcp-$tag.txt" 2>&1
            netstat -I "$iface" -b -n > "$out/netstat-if-$tag.txt" 2>&1
            if [ -n "$pid" ]; then
                procstat -f "$pid" > "$out/procstat-files-$tag.txt" 2>&1
                procstat -r "$pid" > "$out/rusage-$tag.txt" 2>&1
            fi
            ps -ax -o pid,ppid,user,%cpu,%mem,rss,vsz,time,comm 2> /dev/null \
                | awk 'NR == 1 || $NF == "afpd"' > "$out/ps-$tag.txt"
            ;;
        Linux)
            linux_snapshot > "$out/proc-$tag.txt" 2>&1
            ss -tin '( sport = :548 )' > "$out/ss-$tag.txt" 2>&1
            ps -e -o pid,ppid,user:64,%cpu,%mem,rss,vsz,time,comm 2> /dev/null \
                | awk 'NR == 1 || $NF == "afpd"' > "$out/ps-$tag.txt"
            ;;
        *) ;;
    esac
}

snapshot before
date +%s > "$out/times.txt"

# ---- the captures ----

# a background job of this shell ignores SIGINT, so the captures stop on
# SIGTERM; a run cut short stops them too
bg_pids=
trace_pids=
stop_captures() {
    for p in $trace_pids $bg_pids $tcpdump_pid; do
        kill -TERM "$p" 2> /dev/null
    done
}

# -m: each SIGINT to the session toggles maxdebug at its next request, and
# two SIGINTs with no request between them toggle once. afpd writes the log
# and the dircache dumps below /tmp/netatalk-PID, or in /tmp itself where
# that folder is not the session's own. The helper reads only regular files
# of the session's user, never through a link, so nothing another user
# places in /tmp is read or counts. Each SIGINT follows a marker, .toggle1
# or .toggle2, and a toggle's files are those changed since its marker, so
# the files of an earlier run do not count. debug_on is 1 after the first
# SIGINT, 2 after the second, and 0 once the session took both.
debug_on=0
debug_logs() {
    helper_run session-files "$pid" "$uid" logs --since "$out/.toggle1" "$@"
}
debug_dumps() {
    helper_run session-files "$pid" "$uid" dumps "$@"
}
dump_renewed() {
    [ -n "$(debug_dumps --since "$out/.toggle2")" ]
}
abort_run() {
    status=${1:-130}
    stop_captures
    # a session that has ended has no log level to restore
    kill -0 "$pid" 2> /dev/null || debug_on=0
    if [ "$debug_on" -eq 1 ] && [ -n "$(debug_logs)" ]; then
        kill -INT "$pid"
        echo "netatalk-diag: the session returns to its log level at its" \
            "next request" >&2
    elif [ "$debug_on" -eq 1 ]; then
        echo "netatalk-diag: the session switches to maxdebug at its next" \
            "request; after that, run: kill -INT $pid" >&2
    elif [ "$debug_on" -eq 2 ]; then
        echo "netatalk-diag: the session returns to its log level at its" \
            "next request" >&2
    fi
    exit "$status"
}

filter="tcp port 548${client:+ and host $client}"
# -Z root: a tcpdump that gives up root before it opens its file could not
# write in this folder
tcpdump -i "$iface" -s 0 -B 16384 -U -Z root -w "$out/afp.pcap" "$filter" \
    2> "$out/tcpdump.txt" &
tcpdump_pid=$!
trap abort_run INT TERM HUP

# the capture stops at its size cap, which a large copy reaches quickly
cap=$((maxmib * 1048576))
(
    while kill -0 "$tcpdump_pid" 2> /dev/null; do
        size=0
        [ ! -f "$out/afp.pcap" ] || size=$(wc -c < "$out/afp.pcap" | tr -d ' ')
        if [ "$size" -ge "$cap" ]; then
            echo "the capture stopped at its cap of $maxmib MiB" \
                >> "$out/notes.txt"
            : > "$out/.capped"
            kill -TERM "$tcpdump_pid"
            exit 0
        fi
        sleep 1
    done
) &
bg_pids=$!

# dtrace -l reports a description that matches no probe on stderr, below
# its header line, and exits 0
has_probe() {
    probe=$1
    dtrace -l -n "$probe" 2> /dev/null | awk 'NR > 1 {n++} END {exit n == 0}'
}

# bpftrace writes STARTED once its probes are in place, and exits when it
# cannot place them
wait_started() {
    started=$1
    tracer=$2
    i=0
    while [ "$i" -lt 60 ] && ! grep -q STARTED "$started" 2> /dev/null \
        && kill -0 "$tracer" 2> /dev/null; do
        sleep 1
        i=$((i + 1))
    done
}

if [ "$trace" -eq 1 ] && [ "$os" = FreeBSD ]; then
    kldstat -q -m dtraceall || kldload dtraceall
    helper_run program zfs_lookups.d > "$out/zfs_lookups.d"
    # a function with no probe on this kernel reads 0 in the ZFS report too
    grep -o 'fbt::[a-z_]*:entry' "$out/zfs_lookups.d" | sort -u \
        | while read -r p; do
            has_probe "$p" || echo "no probe on this kernel: $p"
        done > "$out/dtrace-zfs.txt"
    if [ "$syscalls" -eq 0 ] && has_probe "afp${pid}:::afpfunc-start"; then
        helper_run program afpd_requests.d > "$out/afpd_requests.d"
        dtrace -q -s "$out/afpd_requests.d" -p "$pid" \
            -o "$out/dtrace-requests.txt" 2> "$out/dtrace-requests.err" &
    else
        [ "$syscalls" -eq 1 ] || note "this afpd has no afp DTrace" \
            "provider: system calls traced in place of requests"
        helper_run program afpd_syscalls.d > "$out/afpd_syscalls.d"
        dtrace -q -s "$out/afpd_syscalls.d" -p "$pid" \
            -o "$out/dtrace-syscalls.txt" 2> "$out/dtrace-syscalls.err" &
    fi
    trace_pids="$!"
    dtrace -q -s "$out/zfs_lookups.d" -o "$out/dtrace-zfs.txt" "$pid" \
        2> "$out/dtrace-zfs.err" &
    trace_pids="$trace_pids $!"
elif [ "$trace" -eq 1 ] && [ "$os" = Linux ]; then
    exe=$(readlink "/proc/$pid/exe")
    if ! command -v bpftrace > /dev/null 2>&1; then
        note "no bpftrace: the session was not traced"
    elif [ ! -d /sys/kernel/tracing/events ] \
        && [ ! -d /sys/kernel/debug/tracing/events ]; then
        note "tracefs is not mounted (mount -t tracefs nodev" \
            "/sys/kernel/tracing): the session was not traced"
    else
        name=afpd_requests
        if [ "$syscalls" -eq 1 ]; then
            name=afpd_syscalls
        elif [ -z "$(bpftrace -l "usdt:$exe:afp:afpfunc__start" \
            2> /dev/null)" ]; then
            note "this afpd has no USDT probes: system calls traced in" \
                "place of requests"
            name=afpd_syscalls
        fi
        kind=${name#afpd_}
        helper_run program "$name.bt" --pid "$pid" --afpd "$exe" \
            > "$out/$name.bt"
        # four times the default ring buffer, for a busy session's REQ lines
        BPFTRACE_PERF_RB_PAGES=256 bpftrace -q -p "$pid" "$out/$name.bt" \
            > "$out/bpftrace-$kind.txt" 2> "$out/bpftrace-$kind.err" &
        trace_pids="$!"
        wait_started "$out/bpftrace-$kind.txt" "$trace_pids"
    fi
fi

# the thread states of the session on Linux, cheap enough for every run;
# perl itself in the background, so SIGTERM reaches it and not a subshell
if [ "$os" = Linux ] && [ -n "$pid" ]; then
    "$perl" "$helper" sample "$pid" "$out/samples.txt" &
    bg_pids="$bg_pids $!"
fi
if [ "$os" = FreeBSD ]; then
    iostat -x -w 1 > "$out/iostat.txt" 2>&1 &
    bg_pids="$bg_pids $!"
fi

if [ "$debug" -eq 1 ]; then
    # the first SIGINT switches the session to maxdebug and dumps the
    # dircache at its next request
    touch "$out/.toggle1"
    kill -INT "$pid"
    debug_on=1
fi

sleep 2
if ! kill -0 "$tcpdump_pid" 2> /dev/null && [ ! -e "$out/.capped" ]; then
    echo "netatalk-diag: tcpdump exited: $(cat "$out/tcpdump.txt")" >&2
    abort_run 1
fi

# Ctrl-C ends the wait, and the run goes on to its reports
stop=0
trap 'stop=1' INT
if [ "$login" -eq 1 ]; then
    echo
    echo "Capturing. Connect the Mac to this server now, then do what is slow."
fi
if [ "$duration" -gt 0 ]; then
    echo "netatalk-diag: capturing for $duration s; Ctrl-C ends it early"
    end=$(($(date +%s) + duration))
    while [ "$stop" -eq 0 ] && [ "$(date +%s)" -lt "$end" ] \
        && kill -0 "$tcpdump_pid" 2> /dev/null; do
        sleep 1
    done
else
    cat << EOF

Capturing. Do what is slow on the Mac now. To mark phases of it, run in
Terminal on the Mac, with VOL the volume's name as /Volumes shows it:

  stat "/Volumes/VOL/navmark-<label>"

Each stat answers "No such file or directory"; that is expected. The
reports keep each label as typed: use one that names nothing private,
such as cold or warm. Press Enter here when done.
EOF
    # Enter or the cap, whichever comes first: the read runs in the
    # background, from the terminal on descriptor 3, so the loop sees both
    exec 3<&0
    (
        read -r _ <&3
        : > "$out/.enter"
    ) &
    reader=$!
    while [ "$stop" -eq 0 ] && [ ! -e "$out/.enter" ] \
        && kill -0 "$tcpdump_pid" 2> /dev/null; do
        sleep 1
    done
    kill "$reader" 2> /dev/null
    exec 3<&-
fi
trap abort_run INT
[ ! -e "$out/.capped" ] || echo "netatalk-diag: the capture stopped at its" \
    "cap of $maxmib MiB"
rm -f "$out/.capped" "$out/.enter"

# the captures and counters cover the wait alone: the -m steps after it wait
# for requests from the Mac
stop_captures
# a trace that found no probe exited long ago, with a status
status=0
for p in $trace_pids; do
    if ! wait "$p"; then
        echo "netatalk-diag: a trace failed; see $out/*.err" >&2
        status=1
    fi
done
# tcpdump stopped by a signal exits 0; a write error does not
if ! wait "$tcpdump_pid"; then
    echo "netatalk-diag: tcpdump failed: $(tail -1 "$out/tcpdump.txt")" >&2
    status=1
fi
wait
date +%s >> "$out/times.txt"

if [ -n "$pid" ] && ! kill -0 "$pid" 2> /dev/null; then
    note "the traced session ended during the run: its counters after the" \
        "run are missing"
fi
snapshot after
if [ -f "$out/datasets.txt" ]; then
    while read -r n ds; do
        zpool iostat -v "${ds%%/*}" > "$out/zpool-iostat-$n-after.txt" 2>&1
    done < "$out/datasets.txt"
fi

if [ "$debug" -eq 1 ]; then
    # the second SIGINT folds into the first unless the session took it; a
    # session that has ended takes neither, and its files stay in /tmp
    while [ -z "$(debug_logs)" ] && kill -0 "$pid" 2> /dev/null; do
        echo "The session has served no request since the first SIGINT." \
            "Run 'ls /Volumes/VOL > /dev/null' on the Mac, then press Enter."
        read -r _
        sleep 2
    done
    if [ -z "$(debug_logs)" ]; then
        note "-m: the session ended before it switched to maxdebug: no log" \
            "and no dircache dumps"
    else
        debug_dumps --since "$out/.toggle1" --copy "$out" --suffix first \
            > /dev/null
        # the second SIGINT dumps again and restores the configured log
        # level, both at the session's next request
        touch "$out/.toggle2"
        kill -INT "$pid" 2> /dev/null
        debug_on=2
        until dump_renewed || ! kill -0 "$pid" 2> /dev/null; do
            echo "Run 'ls /Volumes/VOL > /dev/null' once more on the Mac," \
                "then press Enter."
            read -r _
            sleep 2
        done
        dump_renewed || note "-m: the session ended before its second" \
            "dircache dump"
        debug_dumps --since "$out/.toggle2" --copy "$out" --suffix second \
            > /dev/null
        debug_logs --copy "$out" > /dev/null
    fi
    debug_on=0
    rm -f "$out/.toggle1" "$out/.toggle2"
fi
trap - INT TERM HUP

if [ "$entries" -gt 0 ] && [ -r "$out/afp.conf.raw" ]; then
    echo "netatalk-diag: walking the volumes, at most $entries entries each"
    helper_run baseline "$out/afp.conf.raw" "$entries" > "$out/baseline.txt"
fi
[ "$trace" -eq 1 ] || note "no trace: -t was not given"

echo "netatalk-diag: writing the reports"
helper_run report "$out" "$out/$reports" ${force:+"$force"}
case $? in
    0) ;;
    3)
        echo "netatalk-diag: nothing was packaged. Check the lines above in" \
            "$out/$reports; if they name nothing private, package it with:" >&2
        echo "  tar -czf $out/$reports.tar.gz -C $out $reports" >&2
        exit 3
        ;;
    *) die "the reports failed" ;;
esac

# the archive lives in the folder this run made, where no other user could
# place a path for it; the reports to share are for everyone, in the archive
# too, and the folder opens for traversal only
chmod 755 "$out/$reports"
chmod 644 "$out/$reports"/*
if ! tar -czf "$out/$reports.tar.gz" -C "$out" "$reports"; then
    rm -f "$out/$reports.tar.gz"
    die "packing $out/$reports failed"
fi
chmod 644 "$out/$reports.tar.gz"
chmod 711 "$out"
echo "netatalk-diag: done; send $out/$reports.tar.gz. The raw capture, log" \
    "and dumps stay in $out"
exit "$status"
