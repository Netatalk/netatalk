#!/bin/sh
# PR F verification (scratch branch walk-timing, never merged). Runs inside
# the FreeBSD VM from the synced repository root: the afpd unit tests with
# their volumes on a case-insensitive ZFS dataset, where ochdir() walks
# below the held volume root, then the spectest and lantest with the share
# on a case-insensitive and on a case-sensitive dataset. Results go to
# ./f-results for the artifact upload.
set -eu

OUT="$PWD/f-results"
mkdir -p "$OUT"

truncate -s 6G /fpool.img
zpool create fpool /fpool.img
zfs create -o mountpoint=/mnt/fci -o casesensitivity=insensitive fpool/fci
chmod 1777 /mnt/fci
{
    uname -a
    zfs get -H -o name,property,value casesensitivity fpool/fci
} > "$OUT/system.txt" 2>&1

meson setup build \
    -Dbuildtype=debugoptimized \
    -Dpkg_config_path=/usr/local/libdata/pkgconfig \
    -Dwith-eventloop=libev \
    -Dwith-tests=true \
    -Dwith-testsuite=true > "$OUT/setup.log" 2>&1
meson compile -C build > "$OUT/compile.log" 2>&1
grep -c 'warning:' "$OUT/compile.log" > "$OUT/compile-warnings.txt" || true

for where in fci tmp; do
    if [ "$where" = fci ]; then
        dir=/mnt/fci
    else
        dir=/tmp
    fi

    echo "==== unit tests, volumes in $dir ===="
    rm -f build/meson-logs/testlog.txt
    set +e
    AFPTEST_DIR=$dir meson test -C build --no-rebuild > "$OUT/unit-$where.log" 2>&1
    echo "unit tests in $dir exit $?" | tee -a "$OUT/unit-$where.log"
    set -e
    cp build/meson-logs/testlog.txt "$OUT/unit-testlog-$where.txt" || true
    echo "ok: $(grep -c '^ok ' "$OUT/unit-testlog-$where.txt") not ok: $(grep -c '^not ok' "$OUT/unit-testlog-$where.txt") skip: $(grep -c '# SKIP' "$OUT/unit-testlog-$where.txt")"
    grep -E 'movecwd|names over 255|conversion|dehexed|other kind|deleted name' \
        "$OUT/unit-testlog-$where.txt" | sort | uniq -c || true
done

meson install -C build --no-rebuild > "$OUT/install.log" 2>&1

export NETATALK_CONFDIR=/usr/local/etc
export NETATALK_LOCKDIR=/var/spool/lock
export SERVICE_START="/usr/local/etc/rc.d/netatalk onestart"
export SERVICE_STOP="/usr/local/etc/rc.d/netatalk onestop"
zfs create -o mountpoint=/mnt/afpbackup fpool/backup

for ds in ci cs; do
    if [ "$ds" = ci ]; then
        zfs create -o mountpoint=/mnt/afpshare -o casesensitivity=insensitive \
            fpool/share-$ds
    else
        zfs create -o mountpoint=/mnt/afpshare fpool/share-$ds
    fi

    echo "==== spectest, share case-$ds ===="
    sh ./.github/scripts/vm_spectest.sh > "$OUT/spectest-$ds.txt" 2>&1 \
        || echo "spectest on $ds exited $?" >> "$OUT/spectest-$ds.txt"
    grep -E 'Passed:|Failed:|exited' "$OUT/spectest-$ds.txt" || true
    grep -a -A8 'Failed tests:' "$OUT/spectest-$ds.txt" || true
    echo "==== lantest, share case-$ds ===="
    sh ./.github/scripts/f_lantest.sh > "$OUT/lantest-$ds.txt" 2>&1 \
        || echo "lantest on $ds exited $?" >> "$OUT/lantest-$ds.txt"
    grep -E 'Deep path traversal|exited' "$OUT/lantest-$ds.txt" | tail -2 || true
    mv /var/log/afpd.log "$OUT/afpd-$ds.log" 2> /dev/null || true
    zfs set mountpoint=none fpool/share-$ds
    for cnid in /var/netatalk/CNID /usr/local/var/netatalk/CNID /var/db/netatalk/CNID; do
        if [ -d "$cnid" ]; then
            rm -rf -- "${cnid:?}"
        fi
    done
done

chmod -R a+rX "$OUT"
