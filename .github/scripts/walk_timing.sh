#!/bin/sh
# Walk timing experiment (scratch branch walk-timing, never merged).
# Runs inside the FreeBSD VM after the build/install step, from the synced
# repository root; results go to ./walk-results for the artifact upload.
set -eu

OUT="$PWD/walk-results"
mkdir -p "$OUT"

{
    uname -a
    freebsd-version -kru 2> /dev/null || true
    sysctl -n hw.model hw.ncpu hw.physmem 2> /dev/null || true
    afpd -V 2> /dev/null || true
} > "$OUT/system.txt" 2>&1

truncate -s 6G /afppool.img
zpool create afppool /afppool.img
zfs create -o mountpoint=/mnt/zdef afppool/zdef
zfs create -o mountpoint=/mnt/zci -o casesensitivity=insensitive afppool/zci
zfs create -o mountpoint=/mnt/znfd -o normalization=formD afppool/znfd
mkdir -p /var/tmp/ufs
{
    zfs get -H -o name,property,value casesensitivity,normalization \
        afppool/zdef afppool/zci afppool/znfd
    df -h /mnt/zdef /var/tmp/ufs
} >> "$OUT/system.txt" 2>&1

cc -O2 -Wall -o /tmp/walk_bench test/bench/walk_bench.c

echo "==== walk_bench ===="
for fs in zdef:/mnt/zdef zci:/mnt/zci znfd:/mnt/znfd ufs:/var/tmp/ufs; do
    label=${fs%%:*}
    dir=${fs#*:}
    for depth in 3 8; do
        for sib in 0 2000; do
            /tmp/walk_bench "$dir/b$depth-$sib" "$depth" "$sib" 20000 "$label" \
                | tee -a "$OUT/bench.txt"
        done
    done
done

# afpd under the production spectest, every no-follow ochdir() timed four
# ways: first on a default ZFS dataset, then on a case-insensitive one
touch /tmp/netatalk-ochdir-timing
export NETATALK_CONFDIR=/usr/local/etc
export NETATALK_LOCKDIR=/var/spool/lock
export SERVICE_START="/usr/local/etc/rc.d/netatalk onestart"
export SERVICE_STOP="/usr/local/etc/rc.d/netatalk onestop"
zfs create -o mountpoint=/mnt/afpbackup afppool/afpbackup

for ds in sdef sci; do
    if [ "$ds" = sci ]; then
        zfs create -o mountpoint=/mnt/afpshare -o casesensitivity=insensitive \
            afppool/$ds
    else
        zfs create -o mountpoint=/mnt/afpshare afppool/$ds
    fi

    echo "==== spectest on $ds ===="
    sh ./.github/scripts/vm_spectest.sh > "$OUT/spectest-$ds.txt" 2>&1 \
        || echo "spectest on $ds exited $?" >> "$OUT/spectest-$ds.txt"
    grep -E 'TEST RESULT SUMMARY|Passed:|Failed:|Skipped:' "$OUT/spectest-$ds.txt" || true
    grep 'ochdir timing' /var/log/afpd.log > "$OUT/afpd-timing-$ds.txt" || true
    wc -l "$OUT/afpd-timing-$ds.txt"
    mv /var/log/afpd.log "$OUT/afpd-$ds.log" 2> /dev/null || true
    zfs set mountpoint=none afppool/$ds
    for cnid in /var/netatalk/CNID /usr/local/var/netatalk/CNID /var/db/netatalk/CNID; do
        if [ -d "$cnid" ]; then
            rm -rf -- "${cnid:?}"
        fi
    done
done

chmod -R a+rX "$OUT"
