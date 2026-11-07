#!/bin/sh
# Capture one folder-navigation run of a Mac against a FreeBSD AFP server.
#
# usage: collect.sh -u afpuser -c client-ip [-i iface] [-d dataset]
#                   [-o outdir] [-t [-s]] [-m]
#   -u  the account the Mac logged in with; its afpd session is traced
#   -c  the Mac's address; the packet capture keeps its AFP traffic only
#   -i  the server interface the Mac reaches (default: the default route's)
#   -d  the ZFS dataset of the volume, for its properties
#   -o  output folder (default /var/tmp/navtrace-<date>)
#   -t  trace afpd with DTrace: per-request times through netatalk's afp
#       provider when the build has it, split into time on CPU, asleep and
#       runnable with what afpd slept on, else per-system-call totals; and
#       ZFS name-cache misses and directory scans
#   -s  with -t, the per-system-call totals in place of the request times,
#       whose probes would add to them: a run of its own
#   -m  switch the session to maxdebug logging and dump its dircache, by
#       SIGINT, for the length of the run
#
# Run as root. The script snapshots the configuration and the kernel
# counters, starts the captures, waits for Enter while you navigate on the
# Mac, then stops the captures and snapshots the counters again.
#
# <outdir>/share and <outdir>/share.tar.gz hold the reports to share: no file,
# folder, volume, user or host name and no address is in them. A path keeps
# its depth as /+/+, the request report names folders by per-run tokens, and
# afp.conf keeps its option names with the identifying values removed. The
# raw capture, the afpd log and the dircache dumps stay in <outdir>.

set -u

user=
client=
iface=
dataset=
out=/var/tmp/navtrace-$(date +%Y%m%d-%H%M%S)
trace=0
syscalls=0
debug=0
here=$(cd "$(dirname "$0")" && pwd)

while getopts u:c:i:d:o:tsm opt; do
    case $opt in
        u) user=$OPTARG ;;
        c) client=$OPTARG ;;
        i) iface=$OPTARG ;;
        d) dataset=$OPTARG ;;
        o) out=$OPTARG ;;
        t) trace=1 ;;
        s) syscalls=1 ;;
        m) debug=1 ;;
        *)
            sed -n '3,18p' "$0"
            exit 2
            ;;
    esac
done

if [ -z "$user" ] || [ -z "$client" ]; then
    sed -n '3,18p' "$0"
    exit 2
fi

if [ "$(id -u)" -ne 0 ]; then
    echo "collect.sh: run as root" >&2
    exit 1
fi

if [ -z "$iface" ]; then
    iface=$(route -n get default 2> /dev/null | awk '/interface:/ {print $2}')
fi

# the raw capture, log and dumps hold names: root alone reads them, in a
# folder this run makes itself, so no other user placed anything in it
umask 077
mkdir -p "$(dirname "$out")" || exit 1
if ! mkdir "$out" 2> /dev/null; then
    echo "collect.sh: $out exists; give -o a folder that does not" >&2
    exit 1
fi
echo "collect.sh: writing to $out"

# the session child that serves the Mac: an afpd of the user, connected to
# the client's address on the AFP port
pid=$(sockstat -46 -c -w -P tcp -p 548 2> /dev/null \
    | awk -v u="$user" -v c="$client" \
        '$1 == u && $2 == "afpd" && (index($7, c ":") == 1 || index($7, "[" c "]:") == 1) {print $3; exit}')

if [ -z "$pid" ]; then
    echo "collect.sh: no afpd of $user connected from $client;" \
        "connect the Mac to the volume first" >&2
    exit 1
fi

echo "collect.sh: afpd session pid $pid"

snapshot() {
    tag=$1
    sysctl vfs kern.maxvnodes kstat.zfs.misc.arcstats \
        > "$out/counters-$tag.txt" 2>&1
    vmstat -z > "$out/vmstat-z-$tag.txt" 2>&1
    procstat -f "$pid" > "$out/procstat-files-$tag.txt" 2>&1
    ps -o pid,ppid,%cpu,%mem,rss,vsz,time -p "$pid" > "$out/ps-$tag.txt" 2>&1
}

# the configuration, once
{
    freebsd-version -ku
    uname -srm
    sysctl hw.ncpu hw.physmem kern.maxvnodes vfs.zfs.arc.max \
        vfs.zfs.arc_max 2> /dev/null
} > "$out/system.txt" 2>&1
afpd -V > "$out/afpd-V.txt" 2>&1
grep "DTrace probes" "$out/afpd-V.txt"
conf=$(awk -F':[[:space:]]*' '/afp.conf:/ {print $2; exit}' "$out/afpd-V.txt")
# every section but [Global] and [Homes] loses its name, every comment and
# every line that is no option goes, and only the settings that shape how
# requests are served keep their values
redact_conf() {
    awk '
    /^[[:space:]]*$/ { print; next }
    /^[[:space:]]*\[/ {
        n++
        if (tolower($0) ~ /^[[:space:]]*\[global\]/) {
            print "[Global]"
        } else if (tolower($0) ~ /^[[:space:]]*\[homes\]/) {
            print "[Homes]"
        } else {
            print "[volume " n "]"
        }
        next
    }
    /^[[:space:]]*[^;#[:space:]][^=]*=/ {
        key = tolower($0)
        sub(/[[:space:]]*=.*/, "", key)
        sub(/^[[:space:]]*/, "", key)
        if (key !~ /^(follow symlinks|case sensitive|casefold|ea|cnid scheme|cnid dev|read only|unix charset|vol charset|mac charset|dircache.*|log level|log microseconds|vol size limit|stat vol|spotlight|search db|convert appledouble|delete veto files|invisible dots|legacy icon|time machine|acls|map acls|chmod request|unix priv|strict locking|file perm|directory perm|umask|server quantum|dsireadbuf|tcprcvbuf|tcpsndbuf|use sendfile|splice size|recvfile|preexec close|zeroconf|uam list)$/) {
            sub(/=.*/, "= REDACTED")
        } else {
            sub(/[[:space:]]*[;#].*$/, "")
            sub(/[[:space:]]*\\$/, "")
        }
        print
    }'
}
for f in "$conf" /usr/local/etc/afp.conf /usr/local/etc/netatalk/afp.conf; do
    if [ -n "$f" ] && [ -r "$f" ]; then
        redact_conf < "$f" > "$out/afp.conf"
        break
    fi
done
if [ -n "$dataset" ]; then
    zfs get all "$dataset" > "$out/zfs-dataset.txt" 2>&1
    zpool status "${dataset%%/*}" > "$out/zpool-status.txt" 2>&1
    zpool iostat -v "${dataset%%/*}" > "$out/zpool-iostat-before.txt" 2>&1
fi

snapshot before

# a background job of this shell ignores SIGINT, so the captures stop on
# SIGTERM; a run cut short stops them too
dtrace_pids=
stop_captures() {
    for p in $dtrace_pids $tcpdump_pid; do
        kill -TERM "$p" 2> /dev/null
    done
}

# -m: each SIGINT to the session toggles maxdebug at its next request, and
# two SIGINTs with no request between them toggle once. afpd writes the log
# and the dircache dumps below /tmp/netatalk-PID, or in /tmp itself where
# that folder is not the session's own. debug_on is 1 after the first
# SIGINT, 2 after the second, and 0 once the session took both.
debug_on=0
debug_logs() {
    for f in /tmp/netatalk-"$pid"/afpd."$pid".* /tmp/afpd."$pid".*; do
        [ -f "$f" ] && echo "$f"
    done
}
debug_dumps() {
    for f in /tmp/netatalk-"$pid"/dircache."$pid" /tmp/dircache."$pid"; do
        [ -f "$f" ] && echo "$f"
    done
}
# a dump newer than the marker shows the second toggle taken
dump_renewed() {
    for f in $(debug_dumps); do
        [ -n "$(find "$f" -newer "$out/.toggled")" ] && return 0
    done
    return 1
}
abort_run() {
    status=${1:-130}
    stop_captures
    if [ "$debug_on" -eq 1 ] && [ -n "$(debug_logs)" ]; then
        kill -INT "$pid"
        echo "collect.sh: the session returns to its log level at its" \
            "next request" >&2
    elif [ "$debug_on" -eq 1 ]; then
        echo "collect.sh: the session switches to maxdebug at its next" \
            "request; after that, run: kill -INT $pid" >&2
    elif [ "$debug_on" -eq 2 ]; then
        echo "collect.sh: the session returns to its log level at its" \
            "next request" >&2
    fi
    exit "$status"
}

tcpdump -i "$iface" -s 0 -B 16384 -U -w "$out/afp.pcap" \
    "tcp port 548 and host $client" 2> "$out/tcpdump.txt" &
tcpdump_pid=$!
trap abort_run INT TERM HUP

# dtrace -l reports a description that matches no probe on stderr, below
# its header line, and exits 0
has_probe() {
    dtrace -l -n "$1" 2> /dev/null | awk 'NR > 1 {n++} END {exit n == 0}'
}

if [ "$trace" -eq 1 ]; then
    kldstat -q -m dtraceall || kldload dtraceall
    # a function with no probe on this kernel reads 0 in the ZFS report too;
    # dtrace -o appends to this list of them
    grep -o 'fbt::[a-z_]*:entry' "$here/zfs_lookups.d" | sort -u \
        | while read -r p; do
            has_probe "$p" || echo "no probe on this kernel: $p"
        done > "$out/dtrace-zfs.txt"
    if [ "$syscalls" -eq 0 ] && has_probe "afp${pid}:::afpfunc-start"; then
        dtrace -q -s "$here/afpd_requests.d" -p "$pid" \
            -o "$out/dtrace-requests.txt" 2> "$out/dtrace-requests.err" &
    else
        [ "$syscalls" -eq 0 ] && echo "collect.sh: this afpd has no afp" \
            "provider; tracing system calls only"
        dtrace -q -s "$here/afpd_syscalls.d" -p "$pid" \
            -o "$out/dtrace-syscalls.txt" 2> "$out/dtrace-syscalls.err" &
    fi
    dtrace_pids="$!"
    dtrace -q -s "$here/zfs_lookups.d" -o "$out/dtrace-zfs.txt" "$pid" \
        2> "$out/dtrace-zfs.err" &
    dtrace_pids="$dtrace_pids $!"
fi

if [ "$debug" -eq 1 ]; then
    # the first SIGINT switches the session to maxdebug and dumps the
    # dircache at its next request
    kill -INT "$pid"
    debug_on=1
fi

sleep 2
if ! kill -0 "$tcpdump_pid" 2> /dev/null; then
    echo "collect.sh: tcpdump exited: $(cat "$out/tcpdump.txt")" >&2
    abort_run 1
fi
cat << EOF

Capturing. On the Mac, in Terminal, with VOL replaced by the volume's name
as /Volumes shows it:

  stat "/Volumes/VOL/navmark-down1"   # marks the start of the descent
  ... open the folders one level at a time, down the tree ...
  stat "/Volumes/VOL/navmark-up1"     # marks the start of the ascent
  ... go back up one level at a time (Command-Up) ...
  stat "/Volumes/VOL/navmark-end"     # marks the end

Each stat answers "No such file or directory"; that is expected.
Press Enter here when done.
EOF
read -r _

if [ "$debug" -eq 1 ]; then
    # the second SIGINT folds into the first unless the session took it
    while [ -z "$(debug_logs)" ]; do
        echo "The session has served no request since the first SIGINT." \
            "Run 'ls /Volumes/VOL > /dev/null' on the Mac, then press Enter."
        read -r _
        sleep 2
    done
    for f in $(debug_dumps); do
        cp "$f" "$out/$(basename "$f").first"
    done
    # the second SIGINT dumps again and restores the configured log level,
    # both at the session's next request
    touch "$out/.toggled"
    kill -INT "$pid"
    debug_on=2
    until dump_renewed; do
        echo "Run 'ls /Volumes/VOL > /dev/null' once more on the Mac, then" \
            "press Enter."
        read -r _
        sleep 2
    done
    debug_on=0
    for f in $(debug_dumps); do
        cp "$f" "$out/$(basename "$f").second"
    done
    for f in $(debug_logs); do
        cp "$f" "$out/"
    done
    rm -f "$out/.toggled"
fi

trap - INT TERM HUP
stop_captures
# a DTrace script that found no probe exited long ago, with a status
status=0
for p in $dtrace_pids; do
    if ! wait "$p"; then
        echo "collect.sh: a DTrace capture failed; see $out/dtrace-*.err" >&2
        status=1
    fi
done
wait

snapshot after
if [ -n "$dataset" ]; then
    zpool iostat -v "${dataset%%/*}" > "$out/zpool-iostat-after.txt" 2>&1
fi

if command -v python3 > /dev/null 2>&1; then
    python3 -I "$here/afp_pcap.py" --csv "$out/requests.csv" "$out/afp.pcap" \
        > "$out/report-pcap.txt" 2>&1
    python3 -I "$here/counters_delta.py" "$out/counters-before.txt" \
        "$out/counters-after.txt" > "$out/report-counters.txt" 2>&1
    for f in "$out"/afpd."$pid".*; do
        [ -f "$f" ] && {
            echo "== $(basename "$f")"
            python3 -I "$here/afpd_log.py" "$f"
        } >> "$out/report-log.txt" 2>&1
    done
fi

# the reports to share: a path keeps its depth as /+/+, an address becomes
# ADDR, and the dataset and pool their roles
share=$out/share
mkdir -p "$share"
depth() {
    sed -E 's#/[^/]*#/+#g'
}
addr() {
    sed -E -e 's/[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+(:[0-9]+)?/ADDR/g' \
        -e 's/\[?([0-9a-fA-F]{0,4}:){2,7}[0-9a-fA-F]{0,4}\]?(:[0-9]+)?/ADDR/g'
}
for f in vmstat-z-before.txt vmstat-z-after.txt ps-before.txt ps-after.txt \
    system.txt afp.conf tcpdump.txt report-pcap.txt requests.csv \
    report-counters.txt report-log.txt dtrace-requests.txt \
    dtrace-syscalls.txt dtrace-zfs.txt; do
    [ -f "$out/$f" ] && cp "$out/$f" "$share/$f"
done
# a counter is a number; a string sysctl, as vfs.root.mountfrom, names things
for f in counters-before.txt counters-after.txt; do
    [ -f "$out/$f" ] && awk -F': ' '$2 ~ /^-?[0-9]+$/' "$out/$f" > "$share/$f"
done
# afpd -V names its configuration files and folders: the version and the
# build options stay
grep -v '/' "$out/afpd-V.txt" > "$share/afpd-V.txt"
for f in procstat-files-before.txt procstat-files-after.txt \
    dtrace-requests.err dtrace-syscalls.err dtrace-zfs.err; do
    [ -f "$out/$f" ] && depth < "$out/$f" | addr > "$share/$f"
done
# the dump's ghost lists name an entry without its path
ghosts() {
    sed -E 's/^([0-9]+: +[0-9]+ +[0-9]+ +[0-9]+ [fd] +).* \(GHOST\)$/\1+ (GHOST)/'
}
for f in "$out"/dircache."$pid".*; do
    g=$share/$(basename "$f")
    [ -f "$f" ] && depth < "$f" | ghosts > "$g"
done
if [ -n "$dataset" ]; then
    # the dataset and pool keep their roles in every ZFS file; a dataset
    # property that names a place, a snapshot, a share or a key, or tells
    # the dataset apart, loses its value, a user property goes with its
    # name, an inherited one its ancestor's name; in the pool files a device
    # label reads DEVICE, and an entry of the errors list ENTRY
    strip='function strip(s, needle, repl,    out, i) {
            out = ""
            while (needle != "" && (i = index(s, needle)) > 0) {
                out = out substr(s, 1, i - 1) repl
                s = substr(s, i + length(needle))
            }
            return out s
        }'
    awk -v ds="$dataset" -v pool="${dataset%%/*}" "$strip"'
        $2 ~ /:/ { next }
        $4 == "inherited" { NF = 4 }
        $2 ~ /^(mountpoint|origin|sharenfs|sharesmb|encryptionroot|keylocation|receive_resume_token|redact_snaps|mlslabel|guid)$/ {
            src = $NF
            $3 = "REDACTED"
            $4 = src
            NF = 4
        }
        { print strip(strip($0, ds, "DATASET"), pool, "POOL") }' \
        "$out/zfs-dataset.txt" > "$share/zfs-dataset.txt"
    for f in zpool-status.txt zpool-iostat-before.txt zpool-iostat-after.txt; do
        [ -f "$out/$f" ] && awk -v ds="$dataset" -v pool="${dataset%%/*}" \
            "$strip"'
        /^errors:/ { inerr = 1 }
        inerr && /^[[:space:]]+[^[:space:]]/ { print "        ENTRY"; next }
        {
            for (i = 1; i <= NF; i++) {
                if ($i ~ /\//) {
                    $i = "DEVICE"
                }
            }
            print strip(strip($0, ds, "DATASET"), pool, "POOL")
        }' "$out/$f" > "$share/$f"
    done
fi
cat > "$share/README.txt" << EOF
Folder navigation capture, collect.sh, $(date -u +%Y-%m-%dT%H:%MZ)

No file, folder, volume, user or host name and no address is in these
files. A path keeps its depth as /+/+, the request report names folders
by per-run tokens, afp.conf keeps its option names with the identifying
values removed, the kernel counters are the numeric ones, and the ZFS
dataset and pool are DATASET and POOL.

report-pcap.txt, requests.csv   the AFP requests, per phase and command
report-counters.txt             kernel counter changes over the run
report-log.txt                  the afpd debug log per command and phase (-m)
dircache.*.first, .second       the session's dircache before and after (-m)
dtrace-*.txt                    request times, on CPU and asleep, what afpd
                                slept on, system calls and ZFS lookups (-t)
counters-*, vmstat-z-*, ps-*, procstat-files-*   snapshots before and after
system.txt, afpd-V.txt, afp.conf, zfs-dataset.txt, zpool-*   the configuration
EOF
# the archive lives in the folder this run made, where no other user could
# place a path for it; the reports to share are for everyone, in the archive
# too, and the folder opens for traversal only
chmod 755 "$share"
chmod 644 "$share"/*
tar -czf "$out/share.tar.gz" -C "$out" share
chmod 644 "$out/share.tar.gz"
chmod 711 "$out"
echo "collect.sh: done; share $out/share.tar.gz; the raw capture, log and" \
    "dumps stay in $out"
exit "$status"
