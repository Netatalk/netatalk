#!/bin/sh

# End-to-end Netatalk-Client <-> Samba-Client interoperability test.
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
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
# Exercises the WHOLE dual-protocol chain, both directions, from the
# client layer down:
#
#   Samba-Client (kernel CIFS mount + smbclient)
#     -> SMB -> smbd (vfs_fruit + streams_xattr)
#       -> disk (one shared directory, ea = samba format)
#     <- AFP <- afpd (`multi protocol = yes` + `ea = samba` volume,
#        coherency settings from the multi protocol defaults only)
#   Netatalk-Client (FUSE mount, built from
#     https://github.com/Netatalk/netatalk-client)
#
# The tests are plain file operations on the two mounts, asserting that
# changes made through one client are reflected -- or blocked -- through
# the other, covering each setting `multi protocol = yes` defaults:
#
#   1. strict locking       -- a byte-range lock taken through the CIFS
#                              mount blocks a write through the AFP mount
#                              while held (and only while held)
#   2. deny modes           -- both directions: an AFP deny-mode open
#                              plants the share-mode band and refuses a
#                              conflicting SMB open (fruit:locking =
#                              netatalk); an SMB open should do the same
#                              toward AFP
#   3. dircache validation  -- files created/deleted through the CIFS
#      freq = 1                mount are immediately visible/gone through
#                              the AFP mount (no stale dircache answers)
#   4. rfork/EA coherency   -- an EA already read once through AFP
#                              re-reads fresh after Samba overwrites it
#                              (the staleness the rfork-cache exclusion
#                              guards against)
#   5. DeleteInhibit        -- set over AFP, an SMB delete should refuse
#   6. solaris share        -- Solaris/illumos F_SHARE only; reported as
#      reservations            skipped on Linux
#
# plus content and EA/ADS round-trips in both directions and the on-disk
# trailing-NUL samba format assertion.
#
# Validation points: the Samba client mount (filesystem ops trigger SMB),
# the shared disk (on-disk format, /proc/locks band state), and the
# netatalk-client mount (filesystem ops trigger AFP).  Three AFP-side
# checks use the wire client afparg instead of the FUSE mount because the
# client cannot express them (Netatalk/netatalk-client#310): byte-range
# locks (no FUSE .lock handler -- a fcntl lock on the mount never reaches
# the server), deny-mode opens, and DeleteInhibit.
#
# Checks whose correct semantics Samba does not yet implement print
# "FAILED (Known)" without failing the leg (see known_fail); a Samba-side
# fix flips them to PASS, which flags the marker for removal.
#
# Self-contained container command for the DEBIAN testsuite image: run
# INSTEAD of /entrypoint.sh (docker run --privileged <image>
# /samba_interop_test.sh). The production entrypoint and env_setup
# scripts are reused unchanged. Requires --privileged (FUSE device +
# CIFS kernel mount; the host kernel must have the cifs module).
#
# Required env: AFP_USER, AFP_PASS, AFP_GROUP, SHARE_NAME (the first
# three are hard-required by the sourced env_setup as well)
# Optional: AFP_VERSION (default 7), SMB_PORT (default 445),
#           NETATALK_CLIENT_REPO (default the upstream GitHub URL),
#           NETATALK_CLIENT_REF (default: the repo's latest release tag)

set -u

AFP_VERSION="${AFP_VERSION:-7}"
SMB_PORT="${SMB_PORT:-445}"
NETATALK_CONFDIR="${NETATALK_CONFDIR:-/etc/netatalk}"
SHARE_DIR="${NETATALK_SHARE_DIR:-/mnt/afpshare}"
NETATALK_CLIENT_REPO="${NETATALK_CLIENT_REPO:-https://github.com/Netatalk/netatalk-client.git}"
SMB_UNC="//localhost/${SHARE_NAME}"
AFP_MNT=/mnt/afpclient
SMB_MNT=/mnt/smbclient

PASS_COUNT=0
FAIL_COUNT=0
SKIP_COUNT=0
GAP_COUNT=0

result() {
    # $1 = test name, $2 = 0 (pass) / nonzero (fail), $3 = detail on failure
    if [ "$2" -eq 0 ]; then
        PASS_COUNT=$((PASS_COUNT + 1))
        echo "PASS: $1"
    else
        FAIL_COUNT=$((FAIL_COUNT + 1))
        echo "FAIL: $1 -- ${3:-}"
    fi
}

skip() {
    SKIP_COUNT=$((SKIP_COUNT + 1))
    echo "SKIP: $1 -- $2"
}

# Known failing checks: asserted with the CORRECT expected semantics so a
# fix flips the line to PASS; the failure is printed but its exit-code
# contribution is suppressed until the underlying Samba-side gap closes.
known_fail() {
    # $1 = test name, $2 = 0 (pass) / nonzero (fail), $3 = failure detail
    if [ "$2" -eq 0 ]; then
        PASS_COUNT=$((PASS_COUNT + 1))
        echo "PASS: $1 -- known failure is FIXED; remove its known_fail marker"
    else
        GAP_COUNT=$((GAP_COUNT + 1))
        echo "FAILED (Known): $1 -- ${3:-}"
    fi
}

# True when a POSIX byte lock covering offset 0 exists on the file's
# device:inode -- the non-mutating probe for "is the byte-range lock
# actually held" (both lockers below lock bytes 0-7).
byte_lock_present() {
    # $1 = path
    lock_dev_ino=$(stat -c '%Hd:%Ld:%i' "$1" 2> /dev/null) || return 1
    lock_dev_ino=$(printf '%02x:%02x:%s\n' \
        "${lock_dev_ino%%:*}" "$(echo "$lock_dev_ino" | cut -d: -f2)" \
        "${lock_dev_ino##*:}")
    grep -E "[[:space:]]$lock_dev_ino[[:space:]]" /proc/locks \
        | grep -qE '[[:space:]]0[[:space:]]+(7|EOF)$'
}

# The netatalk share-mode band: ten fixed offsets at OFF_T_MAX - 9 ..
# OFF_T_MAX (AD_FILELOCK base 0x7FFFFFFFFFFFFFF6 = 9223372036854775798,
# through 9223372036854775807).  Match each offset exactly: a plain byte
# lock at BYTELOCK_MAX (...797) is NOT a band entry.
band_present() {
    # $1 = path; true when any band lock exists on its device:inode
    band_dev_ino=$(stat -c '%Hd:%Ld:%i' "$1" 2> /dev/null) || return 1
    band_dev_ino=$(printf '%02x:%02x:%s\n' \
        "${band_dev_ino%%:*}" "$(echo "$band_dev_ino" | cut -d: -f2)" \
        "${band_dev_ino##*:}")
    grep -E "[[:space:]]$band_dev_ino[[:space:]]" /proc/locks \
        | grep -qE '(^|[[:space:]])9223372036854775(79[89]|80[0-7])([[:space:]]|$)'
}

smb() {
    # smbclient one-shot (the ADS/stream view CIFS kernel mounts lack)
    smbclient "$SMB_UNC" "$AFP_PASS" -U "$AFP_USER" -p "$SMB_PORT" -c "$1"
}

# Run "$@" and report success when it FAILS (blocked/refused/denied is
# the expected outcome).  Keeps every flag in pass-polarity: 0 = the
# assertion holds.
expect_blocked() {
    if "$@" > /dev/null 2>&1; then
        return 1
    fi

    return 0
}

# Poll a condition command (up to $1 seconds, 1s interval); 0 when it
# became true.  One synchronisation idiom for every hold/release wait.
wait_for() {
    wait_secs=$1
    shift

    for _ in $(seq 1 "$wait_secs"); do
        if "$@" > /dev/null 2>&1; then
            return 0
        fi

        sleep 1
    done

    return 1
}

# --------------------------------------------------------------------------
# Test dependencies (kept out of the shared testsuite image: only this leg
# needs a Samba server, SMB clients, FUSE, and the netatalk-client build)
# --------------------------------------------------------------------------

echo "*** Installing samba, clients, and netatalk-client build deps"
export DEBIAN_FRONTEND=noninteractive
apt-get update -qq
apt-get install -qq --yes --no-install-recommends \
    samba smbclient cifs-utils attr python3 fuse3 procps \
    git ca-certificates meson ninja-build pkg-config build-essential \
    libgcrypt20-dev libreadline-dev libfuse3-dev > /dev/null || {
    echo "FATAL: package installation failed"
    exit 1
}

# --------------------------------------------------------------------------
# Netatalk-Client: pull from GitHub and build (FUSE enabled)
# --------------------------------------------------------------------------

# Build the latest RELEASE of netatalk-client, not a development
# snapshot: the leg tests the published client against this server.
# NETATALK_CLIENT_REF overrides (any tag or branch) for pinning or for
# testing an unreleased client.
if [ -z "${NETATALK_CLIENT_REF:-}" ]; then
    NETATALK_CLIENT_REF=$(git ls-remote --tags --sort=-v:refname \
        "$NETATALK_CLIENT_REPO" \
        | grep -v '\^{}' | head -1 | sed 's|.*refs/tags/||')
fi

if [ -z "$NETATALK_CLIENT_REF" ]; then
    echo "FATAL: could not resolve a netatalk-client release tag"
    exit 1
fi

echo "*** Building netatalk-client $NETATALK_CLIENT_REF from $NETATALK_CLIENT_REPO"
git clone --quiet --depth 1 --branch "$NETATALK_CLIENT_REF" \
    "$NETATALK_CLIENT_REPO" /tmp/netatalk-client
(
    cd /tmp/netatalk-client || exit 1
    meson setup build --buildtype=release > /dev/null
    meson compile -C build > /dev/null
    meson install -C build > /dev/null
) || {
    echo "FATAL: netatalk-client build failed"
    exit 1
}
# meson installs to /usr/local; register libafpclient with the loader
ldconfig
command -v afp_client > /dev/null 2>&1 || export PATH="$PATH:/usr/local/bin"

# --------------------------------------------------------------------------
# afp.conf: the one-switch story -- a `multi protocol = yes` volume with
# the ea = samba storage format, no strict-locking / dircache keys
# anywhere; the stock entrypoint then just serves (MANUAL_CONFIG keeps
# this config; no TESTSUITE set)
# --------------------------------------------------------------------------

echo "*** Writing afp.conf (multi protocol = yes, ea = samba) and starting netatalk"
mkdir -p "$NETATALK_CONFDIR"
cat << EOF > "$NETATALK_CONFDIR/afp.conf"
[Global]
afp listen = 0.0.0.0
log file = /var/log/afpd.log
log level = default:note
uam list = uams_clrtxt.so uams_dhx2.so

[${SHARE_NAME}]
cnid scheme = sqlite
path = ${SHARE_DIR}
ea = samba
multi protocol = yes
valid users = ${AFP_USER}
volume name = ${SHARE_NAME}
EOF

MANUAL_CONFIG=1 TESTSUITE= /entrypoint.sh &

AFP_UP=1
for _ in $(seq 1 30); do
    if grep -q 'AFP/TCP listening' /var/log/afpd.log 2> /dev/null; then
        AFP_UP=0
        break
    fi

    sleep 1
done

if [ "$AFP_UP" -ne 0 ]; then
    echo "FATAL: afpd did not start listening on 548"
    tail -50 /var/log/afpd.log 2> /dev/null
    exit 1
fi

# --------------------------------------------------------------------------
# smb.conf per the manual's Interoperability section, on the same volume
# --------------------------------------------------------------------------

echo "*** Writing smb.conf and starting smbd"
mkdir -p /var/log/samba /var/lib/samba/private /run/samba /etc/samba
cat << EOF > /etc/samba/smb.conf
[global]
    ea support = yes
    vfs objects = catia fruit streams_xattr

    fruit:encoding = native
    fruit:locking = netatalk
    streams_xattr:prefix = user.
    streams_xattr:store_stream_type = no

    strict locking = yes
    ; posix locking = yes is the Samba default; do not disable it

    oplocks = no
    level2 oplocks = no
    smb2 leases = no

    create mask = 0664
    directory mask = 0775
    map archive = no

    security = user
    map to guest = never
    smb ports = ${SMB_PORT}
    log file = /var/log/samba/smbd.log
    log level = 1

[${SHARE_NAME}]
    path = ${SHARE_DIR}
    read only = no
    valid users = ${AFP_USER}
EOF

# Samba keeps its own password database; mirror the AFP user into it.
printf '%s\n%s\n' "$AFP_PASS" "$AFP_PASS" | smbpasswd -a -s "$AFP_USER" || {
    echo "FATAL: smbpasswd could not add $AFP_USER"
    exit 1
}
smbd --daemon

SMB_UP=1
for _ in $(seq 1 15); do
    if smb 'ls' > /dev/null 2>&1; then
        SMB_UP=0
        break
    fi

    sleep 1
done

if [ "$SMB_UP" -ne 0 ]; then
    echo "FATAL: smbd did not come up or share \"$SHARE_NAME\" is not browsable"
    tail -50 /var/log/samba/smbd.log 2> /dev/null
    exit 1
fi

# --------------------------------------------------------------------------
# The two client mounts
# --------------------------------------------------------------------------

echo "*** Mounting the share via both protocol clients"
mkdir -p "$AFP_MNT" "$SMB_MNT"

# Samba-Client: kernel CIFS mount. actimeo=0 turns off client-side
# attribute caching so every operation exercises the wire (our smb.conf
# already disables oplocks/leases server-side).
mount -t cifs "$SMB_UNC" "$SMB_MNT" \
    -o "user=${AFP_USER},pass=${AFP_PASS},port=${SMB_PORT},vers=3.1.1,actimeo=0,noserverino" || {
    echo "FATAL: CIFS mount failed (host kernel lacks the cifs module?)"
    exit 1
}

# Netatalk-Client: FUSE mount (afp_client starts afpfsd on demand).
afp_client mount --user "$AFP_USER" --pass "$AFP_PASS" \
    "localhost:${SHARE_NAME}" "$AFP_MNT"

AFP_MNT_UP=1
for _ in $(seq 1 15); do
    if mount | grep -q " $AFP_MNT "; then
        AFP_MNT_UP=0
        break
    fi

    sleep 1
done

if [ "$AFP_MNT_UP" -ne 0 ]; then
    echo "FATAL: AFP FUSE mount did not appear"
    exit 1
fi

# --------------------------------------------------------------------------
# Control: the coherency behaviour below must come from the multi
# protocol DEFAULTS -- assert no explicit strict-locking/dircache key
# crept into the config, and that afpd logged applying the default
# --------------------------------------------------------------------------

if grep -Eq '^[[:space:]]*(strict locking|afp read locks|dircache validation freq)[[:space:]]*=' \
    "$NETATALK_CONFDIR/afp.conf"; then
    result "control: coherency settings are defaults, not explicit config" 1 \
        "afp.conf must carry only multi protocol = yes and ea = samba"
else
    result "control: coherency settings are defaults, not explicit config" 0
fi

grep -q "defaulting 'strict locking' to yes" /var/log/afpd.log
result "control: afpd logged the multi protocol strict-locking default" $? \
    "expected the multi protocol defaults log_note in /var/log/afpd.log"

# --------------------------------------------------------------------------
# 1. Content: Netatalk-Client writes, Samba-Client reads
# --------------------------------------------------------------------------

A2S="via-netatalk-client-$$"
printf '%s' "$A2S" > "$AFP_MNT/chain_afp.txt"
[ "$(cat "$SMB_MNT/chain_afp.txt" 2> /dev/null)" = "$A2S" ]
result "content: AFP-mount write -> SMB-mount read" $? \
    "read back: '$(cat "$SMB_MNT/chain_afp.txt" 2> /dev/null)'"

# --------------------------------------------------------------------------
# 2. Content: Samba-Client writes, Netatalk-Client reads
# --------------------------------------------------------------------------

S2A="via-samba-client-$$"
printf '%s' "$S2A" > "$SMB_MNT/chain_smb.txt"
[ "$(cat "$AFP_MNT/chain_smb.txt" 2> /dev/null)" = "$S2A" ]
result "content: SMB-mount write -> AFP-mount read" $? \
    "read back: '$(cat "$AFP_MNT/chain_smb.txt" 2> /dev/null)'"

# --------------------------------------------------------------------------
# 3. Directory-listing coherency, both directions: names created/removed
#    through one mount are immediately reflected through the other (the
#    AFP-visible half is the dircache validation freq = 1 default; the
#    SMB-visible half is Samba's kernel change notify + actimeo=0)
# --------------------------------------------------------------------------

touch "$SMB_MNT/dircache_probe.txt"
ls "$AFP_MNT" | grep -q '^dircache_probe.txt$'
result "dircache default: SMB-created name immediately listed over AFP" $? \
    "freq=1 default must revalidate on every access"

rm "$SMB_MNT/dircache_probe.txt"
! ls "$AFP_MNT" | grep -q '^dircache_probe.txt$'
result "dircache default: SMB-deleted name immediately gone over AFP" $? \
    "stale dircache entry served after external delete"

touch "$AFP_MNT/dircache_probe2.txt"
ls "$SMB_MNT" | grep -q '^dircache_probe2.txt$'
result "listing mirror: AFP-created name immediately listed over SMB" $? \
    "SMB listing missed a file created over AFP"

rm "$AFP_MNT/dircache_probe2.txt"
! ls "$SMB_MNT" | grep -q '^dircache_probe2.txt$'
result "listing mirror: AFP-deleted name immediately gone over SMB" $? \
    "stale SMB listing served after AFP delete"

# --------------------------------------------------------------------------
# 4. EA: Netatalk-Client sets via FUSE xattr; Samba-Client reads the
#    matching alternate data stream; on-disk value carries the samba
#    trailing NUL
# --------------------------------------------------------------------------

setfattr -n user.chain_attr -v "ea-via-afp" "$AFP_MNT/chain_afp.txt"
smb 'get chain_afp.txt:chain_attr /tmp/chain_ads.txt' > /dev/null 2>&1
[ "$(cat /tmp/chain_ads.txt 2> /dev/null)" = "ea-via-afp" ]
result "EA: AFP-mount setfattr -> SMB stream read" $? \
    "ADS read back: '$(cat /tmp/chain_ads.txt 2> /dev/null)'"

ONDISK_HEX=$(getfattr -e hex -n user.chain_attr \
    "$SHARE_DIR/chain_afp.txt" 2> /dev/null \
    | sed -n 's/^user\.chain_attr=//p')

case "$ONDISK_HEX" in
    0x*00)
        result "EA: on-disk value carries the samba trailing NUL" 0
        ;;

    *)
        result "EA: on-disk value carries the samba trailing NUL" 1 \
            "getfattr hex: '$ONDISK_HEX'"
        ;;
esac

# --------------------------------------------------------------------------
# 5. ADS: Samba-Client writes a stream; Netatalk-Client reads the EA
# --------------------------------------------------------------------------

printf 'ads-via-smb' > /tmp/chain_ads_put.txt
smb 'put /tmp/chain_ads_put.txt chain_smb.txt:smb_attr' > /dev/null 2>&1
[ "$(getfattr --only-values -n user.smb_attr "$AFP_MNT/chain_smb.txt" 2> /dev/null)" = "ads-via-smb" ]
result "EA: SMB stream write -> AFP-mount getfattr" $? \
    "EA read back: '$(getfattr --only-values -n user.smb_attr "$AFP_MNT/chain_smb.txt" 2> /dev/null)'"

# --------------------------------------------------------------------------
# 6. EA coherency, both directions: an EA already read once through one
#    protocol re-reads FRESH after the other overwrites it (the AFP half
#    is what the rfork-cache exclusion guards)
# --------------------------------------------------------------------------

getfattr --only-values -n user.smb_attr "$AFP_MNT/chain_smb.txt" > /dev/null 2>&1
printf 'ads-overwritten' > /tmp/chain_ads_put2.txt
smb 'put /tmp/chain_ads_put2.txt chain_smb.txt:smb_attr' > /dev/null 2>&1
[ "$(getfattr --only-values -n user.smb_attr "$AFP_MNT/chain_smb.txt" 2> /dev/null)" = "ads-overwritten" ]
result "coherency: Samba-overwritten EA re-reads fresh over AFP" $? \
    "stale EA served after external change"

smb 'get chain_afp.txt:interop_attr /tmp/chain_mirror1.txt' > /dev/null 2>&1
setfattr -n user.interop_attr -v "ea-overwritten" "$AFP_MNT/chain_afp.txt"
smb 'get chain_afp.txt:interop_attr /tmp/chain_mirror2.txt' > /dev/null 2>&1
[ "$(cat /tmp/chain_mirror2.txt 2> /dev/null)" = "ea-overwritten" ]
result "coherency mirror: AFP-overwritten EA re-reads fresh over SMB" $? \
    "stale stream served after AFP-side change: '$(cat /tmp/chain_mirror2.txt 2> /dev/null)'"

# --------------------------------------------------------------------------
# 7. Strict locking, both directions with the same equivalent operations:
#    a byte-range lock held through one protocol blocks a write through
#    the other while held, and only while held.  SMB -> AFP: the CIFS
#    lock is mirrored to POSIX by smbd's posix locking, and afpd's
#    per-write F_WRLCK (the strict locking default) conflicts.  AFP ->
#    SMB: the FPByteRangeLock is a real POSIX lock, and smbd's strict
#    locking = yes checks it on the SMB write.
# --------------------------------------------------------------------------

printf 'lock-target-content' > "$SMB_MNT/lock_a.txt"
rm -f /tmp/locked.flag
python3 - "$SMB_MNT/lock_a.txt" << 'PYEOF' &
import fcntl
import sys
import time

f = open(sys.argv[1], "r+b")
fcntl.lockf(f, fcntl.LOCK_EX)
open("/tmp/locked.flag", "w").close()
time.sleep(300)
PYEOF
LOCKER_PID=$!

if ! wait_for 10 test -f /tmp/locked.flag; then
    result "strict locking: SMB-held lock blocks AFP-mount write" 1 \
        "CIFS-side locker never took its lock"
else
    expect_blocked dd if=/dev/zero of="$AFP_MNT/lock_a.txt" bs=8 count=1 \
        conv=notrunc
    result "strict locking: SMB-held lock blocks AFP-mount write" $? \
        "AFP write succeeded while the SMB byte-range lock was held"
fi

kill "$LOCKER_PID" 2> /dev/null
wait "$LOCKER_PID" 2> /dev/null
wait_for 15 dd if=/dev/zero of="$AFP_MNT/lock_a.txt" bs=8 count=1 \
    conv=notrunc
result "strict locking: AFP-mount write succeeds after lock release" $? \
    "release-side control failed: write still blocked"

# Mirror direction: an AFP byte-range lock (held via afparg over the
# wire) blocks a write through the CIFS mount, and only while held.
printf 'lock-target-content' > "$AFP_MNT/lock_m.txt"
afparg -"$AFP_VERSION" -h localhost -u "$AFP_USER" -w "$AFP_PASS" \
    -s "$SHARE_NAME" -f FPByteLockHold d lock_m.txt 10 > /dev/null 2>&1 &
AFPLOCK_PID=$!

# Gate on the lock actually existing (mirrors the /tmp/locked.flag gate in
# the other direction) so an afparg failure reads as setup, not coherency.
if ! wait_for 8 byte_lock_present "$SHARE_DIR/lock_m.txt"; then
    result "strict locking mirror: AFP-held lock blocks SMB-mount write" 1 \
        "AFP-side locker never took its byte-range lock"
else
    expect_blocked dd if=/dev/zero of="$SMB_MNT/lock_m.txt" bs=8 count=1 \
        conv=notrunc
    result "strict locking mirror: AFP-held lock blocks SMB-mount write" $? \
        "SMB write succeeded while the AFP byte-range lock was held"
fi

wait "$AFPLOCK_PID" 2> /dev/null
wait_for 15 dd if=/dev/zero of="$SMB_MNT/lock_m.txt" bs=8 count=1 \
    conv=notrunc
result "strict locking mirror: SMB-mount write succeeds after lock release" \
    $? "release-side control failed: SMB write still blocked"

# --------------------------------------------------------------------------
# 8. Deny modes, AFP -> SMB: an AFP deny-read/write open plants the
#    share-mode band on disk (asserted via /proc/locks) and makes Samba
#    refuse a conflicting SMB write open with NT_STATUS_SHARING_VIOLATION
#    via fruit:locking = netatalk -- until released
# --------------------------------------------------------------------------

printf 'deny-target-content' > "$AFP_MNT/lock_b.txt"
# Timed hold (10s), then afparg closes the fork and logs out cleanly --
# a killed AFP session would park disconnected and KEEP its share modes.
afparg -"$AFP_VERSION" -h localhost -u "$AFP_USER" -w "$AFP_PASS" \
    -s "$SHARE_NAME" -f FPLockrw d lock_b.txt 10 > /dev/null 2>&1 &
DENY_PID=$!

wait_for 8 band_present "$SHARE_DIR/lock_b.txt"
result "deny AFP->SMB: AFP open plants the on-disk share-mode band" $? \
    "no netatalk band locks visible in /proc/locks"

printf 'overwrite-attempt' > /tmp/deny_put.txt
smb 'put /tmp/deny_put.txt lock_b.txt' 2>&1 \
    | grep -q 'NT_STATUS_SHARING_VIOLATION'
result "deny AFP->SMB: Samba-Client write refused (sharing violation)" $? \
    "expected NT_STATUS_SHARING_VIOLATION while the AFP deny open is held"

# Known failure: the same conflicting write through the kernel CIFS
# mount should also be refused, but its open pattern slips past smbd's
# share-mode check that smbclient correctly trips.
expect_blocked dd if=/dev/zero of="$SMB_MNT/lock_b.txt" bs=8 count=1 \
    conv=notrunc
known_fail "deny AFP->SMB: kernel-CIFS write refused (sharing violation)" $? \
    "the CIFS write succeeded and landed on disk (share-mode check bypassed)"

wait "$DENY_PID" 2> /dev/null
wait_for 15 smb 'put /tmp/deny_put.txt lock_b.txt'
result "deny AFP->SMB: Samba-Client write succeeds after AFP close" $? \
    "release-side control failed: write still refused after close+logout"

# --------------------------------------------------------------------------
# 9. Deny modes, SMB -> AFP (the symmetric direction): a read-write open
#    held through the CIFS mount plants the band (fruit only places its
#    markers for opens with read access -- a write-only open is invisible
#    to AFP by Samba's design), and a conflicting AFP deny-read/write
#    open must be refused -- until the SMB open closes
# --------------------------------------------------------------------------

printf 'smb-held-content' > "$SMB_MNT/lock_c.txt"
(
    exec 3<> "$SMB_MNT/lock_c.txt"
    sleep 10
) &
SMB_HOLD_PID=$!

# The band never appears today (the known failure below), so poll briefly
# for it and move on: the window both bounds the flake risk if Samba ever
# plants it late and keeps the leg fast while the gap persists.
wait_for 3 band_present "$SHARE_DIR/lock_c.txt"
known_fail "deny SMB->AFP: SMB read-write open plants the on-disk band" $? \
    "fruit:locking = netatalk planted no band locks for the CIFS open"

# Probe sanity first: an unrelated AFP failure (login, volume) must read
# as setup breakage, not as the deny gap being fixed.
if ! afparg -"$AFP_VERSION" -h localhost -u "$AFP_USER" -w "$AFP_PASS" \
    -s "$SHARE_NAME" -f FPRead lock_c.txt > /dev/null 2>&1; then
    result "deny SMB->AFP: AFP probe session sanity" 1 \
        "afparg cannot reach the server; deny probe skipped"
else
    expect_blocked afparg -"$AFP_VERSION" -h localhost -u "$AFP_USER" \
        -w "$AFP_PASS" -s "$SHARE_NAME" -f FPLockrw d lock_c.txt 1
    known_fail "deny SMB->AFP: AFP deny-mode open refused while SMB open held" \
        $? "the AFP open succeeded despite the held SMB open"
fi

wait "$SMB_HOLD_PID" 2> /dev/null
# Each iteration is a full AFP session with a 1s hold (~2-3s), so 5
# retries bounds this at ~15s wall time.
wait_for 5 afparg -"$AFP_VERSION" -h localhost -u "$AFP_USER" \
    -w "$AFP_PASS" -s "$SHARE_NAME" -f FPLockrw d lock_c.txt 1
result "deny SMB->AFP: AFP deny-mode open succeeds after SMB close" $? \
    "release-side control failed: AFP open still refused"

# --------------------------------------------------------------------------
# 10. DeleteInhibit: set over AFP, an SMB delete should be refused until
#     the attribute is cleared.  Known failure: smbd deletes the file
#     regardless (kFPDeleteInhibitBit lives in netatalk's metadata EA,
#     which vfs_fruit reads for FinderInfo but does not consult for
#     deletes).
# --------------------------------------------------------------------------

printf 'inhibit-me' > "$AFP_MNT/lock_d.txt"
afparg -"$AFP_VERSION" -h localhost -u "$AFP_USER" -w "$AFP_PASS" \
    -s "$SHARE_NAME" -f FPSetInhibit lock_d.txt on > /dev/null 2>&1
result "DeleteInhibit: attribute set over AFP" $? \
    "FPSetFileParams failed"

smb 'del lock_d.txt' > /dev/null 2>&1
test -f "$SHARE_DIR/lock_d.txt"
known_fail "DeleteInhibit: SMB delete refused while attribute set" $? \
    "smbd deleted the file despite the AFP DeleteInhibit attribute"

if [ -f "$SHARE_DIR/lock_d.txt" ]; then
    afparg -"$AFP_VERSION" -h localhost -u "$AFP_USER" -w "$AFP_PASS" \
        -s "$SHARE_NAME" -f FPSetInhibit lock_d.txt off > /dev/null 2>&1
    smb 'del lock_d.txt' > /dev/null 2>&1
    [ ! -f "$SHARE_DIR/lock_d.txt" ]
    result "DeleteInhibit: SMB delete succeeds after attribute cleared" $? \
        "release-side control failed: delete still refused"
fi

# --------------------------------------------------------------------------
# 11. Solaris share reservations: the fourth multi protocol coherency setting
# --------------------------------------------------------------------------

skip "solaris share reservations (F_SHARE deny-mode layer)" \
    "Solaris/illumos only; not testable on a Linux kernel"

# --------------------------------------------------------------------------
# Summary
# --------------------------------------------------------------------------

echo ""
echo "==== CLIENT-CHAIN INTEROP: $PASS_COUNT passed, $FAIL_COUNT failed, $SKIP_COUNT skipped, $GAP_COUNT known failures ===="

# Teardown after the summary so a wedged unmount cannot eat the results;
# bounded so it cannot hang the job either.
timeout 30 umount "$SMB_MNT" 2> /dev/null
timeout 30 afp_client unmount "$AFP_MNT" 2> /dev/null

if [ "$FAIL_COUNT" -ne 0 ]; then
    echo "==== smbd log tail ===="
    tail -50 /var/log/samba/smbd.log 2> /dev/null
    echo "==== afpd log tail ===="
    tail -80 /var/log/afpd.log 2> /dev/null
    exit 1
fi

exit 0
