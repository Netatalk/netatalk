#!/bin/sh
# Walk timing experiment (scratch branch walk-timing, never merged): the Linux
# leg. Runs on the runner as root from the repository root, after the
# testsuite image is built as netatalk-walk; results go to ./walk-results.
set -eu

OUT="$PWD/walk-results"
mkdir -p "$OUT" /mnt/w

{
    uname -a
    nproc
    grep -m1 'model name' /proc/cpuinfo || true
} > "$OUT/system.txt" 2>&1

for fs in ext4 xfs btrfs; do
    truncate -s 3G "/tmp/$fs.img"
    case $fs in
        ext4) mkfs.ext4 -q -F "/tmp/$fs.img" ;;
        xfs) mkfs.xfs -q -f "/tmp/$fs.img" ;;
        btrfs) mkfs.btrfs -q -f "/tmp/$fs.img" ;;
    esac
    mkdir -p "/mnt/w/$fs"
    mount -o loop "/tmp/$fs.img" "/mnt/w/$fs"
done

mkdir -p /mnt/w/tmpfs
mount -t tmpfs -o size=3G tmpfs /mnt/w/tmpfs
FILESYSTEMS="ext4 xfs btrfs tmpfs"

if modprobe zfs; then
    truncate -s 6G /tmp/zfs.img
    zpool create wpool /tmp/zfs.img
    zfs create -o mountpoint=/mnt/w/zdef wpool/zdef
    zfs create -o mountpoint=/mnt/w/zci -o casesensitivity=insensitive wpool/zci
    FILESYSTEMS="$FILESYSTEMS zdef zci"
fi

{
    df -hT /mnt/w/*
    zfs get -H -o name,property,value casesensitivity wpool/zdef wpool/zci || true
} >> "$OUT/system.txt" 2>&1

gcc -O2 -Wall -D_GNU_SOURCE -o /tmp/walk_bench test/bench/walk_bench.c

# DEPTH is the number of folders between the volume root and the two
# leaves, so the leaves sit 1, 2, 4 and 8 levels below the root
echo "==== walk_bench ===="
for fs in $FILESYSTEMS; do
    for depth in 0 1 3 7; do
        for sib in 0 2000; do
            /tmp/walk_bench "/mnt/w/$fs/b$depth-$sib" "$depth" "$sib" 20000 "$fs" \
                | tee -a "$OUT/bench.txt"
        done
    done
done

# afpd in CI's Alpine test image with the VM legs' production tuning, every
# no-follow ochdir() timed five ways, under the spectest and under lantest,
# its share on each filesystem
for fs in $FILESYSTEMS; do
    for suite in spec lan; do
        share="/mnt/w/$fs/$suite-share"
        backup="/mnt/w/$fs/$suite-backup"
        mkdir -p "$share" "$backup"
        echo "==== $suite on $fs ===="
        docker run --rm \
            -v "$share:/mnt/afpshare" -v "$backup:/mnt/afpbackup" \
            -e AFP_USER=atalk1 -e AFP_USER2=atalk2 \
            -e AFP_PASS=afppass1 -e AFP_PASS2=afppass1 \
            -e AFP_GROUP=afpusers -e SHARE_NAME=test1 -e SHARE_NAME2=test2 \
            -e INSECURE_AUTH=1 -e DISABLE_TIMEMACHINE=1 \
            -e TESTSUITE="$suite" -e VERBOSE=1 -e AFP_VERSION=7 -e AFP_EXTMAP=1 \
            -e AFP_DIRCACHESIZE=1048576 -e AFP_DIRCACHE_MODE=arc \
            -e AFP_DIRCACHE_VALIDATION_FREQ=100 -e AFP_CNID_BACKEND=sqlite \
            -e AFP_CONVERT_APPLEDOUBLE=no -e AFP_TEST_ITERATIONS=1 \
            -e NETATALK_OCHDIR_TIMING=1 -e SERVER_LOGS=1 \
            netatalk-walk > "$OUT/$suite-$fs.txt" 2>&1 \
            || echo "$suite on $fs exited $?" >> "$OUT/$suite-$fs.txt"
        grep -E 'Passed:|Failed:|exited' "$OUT/$suite-$fs.txt" || true
        grep 'ochdir timing' "$OUT/$suite-$fs.txt" \
            > "$OUT/afpd-timing-$suite-$fs.txt" || true
        wc -l "$OUT/afpd-timing-$suite-$fs.txt"
    done
done

chmod -R a+rX "$OUT"
