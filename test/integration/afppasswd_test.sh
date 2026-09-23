#!/bin/sh
# Integration tests for the non-root private SRP verifier bootstrap.
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

id afptest > /dev/null 2>&1 || adduser -D afptest
SYSVER=$(netatalk_confdir)/afppasswd.srp
VERDIR=/home/afptest/.config/netatalk
VER=$VERDIR/afppasswd.srp
UIDFILE=$VER/$(id -u afptest)
# A password cracklib accepts, so the script also runs on an image built with
# it; a non-root caller cannot pass -n.
PASS=Kw7vTq9zLp2rXm4b

# --- 1: a user creates a verifier directory holding only themselves
su afptest -c "mkdir -p -m 700 $VERDIR"
su afptest -c "afppasswd -c -p $VER -w $PASS" \
    || fail "non-root create of own verifier failed"
owned "$VER" "afptest 700" \
    || fail "private verifier directory is not afptest-owned mode 0700"
owned "$UIDFILE" "afptest 600" \
    || fail "private verifier is not afptest-owned mode 0600"
awk -F: '$1 == "afptest" && length($2) == 32 && length($3) == 384 \
    && ($2 $3) ~ /^[0-9A-F]+$/ { ok = 1 } END { exit !ok }' "$UIDFILE" \
    || fail "private verifier holds no real record for afptest"
[ "$(wc -l < "$UIDFILE")" -eq 1 ] \
    || fail "private verifier holds more than one record"
[ "$(ls "$VER" | wc -l)" -eq 1 ] \
    || fail "private verifier directory holds more than one entry"
pass "non-root create of a private verifier directory"

# --- 2: non-root -c without -p is refused by policy: the default path is
# the shared system store
su afptest -c "afppasswd -c -w $PASS" > /tmp/afppasswd_out 2>&1 \
    && fail "non-root create without -p succeeded"
grep -q 'non-root -c requires -p' /tmp/afppasswd_out \
    || fail "refusal is not the explicit-path policy message"
[ ! -e "$SYSVER/$(id -u afptest)" ] \
    || fail "non-root create without -p wrote to the system store"
pass "non-root create without -p refused by policy"

# --- 3: a cancelled prompt under -f must leave the record from leg 1
# intact. getpass() reads the controlling terminal and setsid starts the
# command in a session that has none, so the first prompt fails and
# afppasswd takes the same abort path as a rejected or mismatched entry.
SUM_BEFORE=$(sha256sum "$UIDFILE" | cut -d' ' -f1)
setsid su afptest -c "afppasswd -c -f -p $VER" < /dev/null \
    > /tmp/afppasswd_out 2>&1 \
    && fail "cancelled -f create reported success"
grep -q 'password input canceled' /tmp/afppasswd_out \
    || fail "cancelled -f create did not take the cancel path"
SUM_AFTER=$(sha256sum "$UIDFILE" | cut -d' ' -f1)
[ "$SUM_BEFORE" = "$SUM_AFTER" ] \
    || fail "aborted -f create destroyed the verifier ($(wc -c < "$UIDFILE") bytes left)"
pass "aborted -f create leaves the previous verifier intact"

# --- 4: a cancelled prompt on a fresh directory must leave no <uid> file
# behind, so the retry needs no -f
VER2=$VERDIR/afppasswd2.srp
setsid su afptest -c "afppasswd -c -p $VER2" < /dev/null \
    > /tmp/afppasswd_out 2>&1 \
    && fail "cancelled create reported success"
grep -q 'password input canceled' /tmp/afppasswd_out \
    || fail "cancelled create did not take the cancel path"
[ ! -e "$VER2/$(id -u afptest)" ] \
    || fail "aborted create left an empty verifier behind"
su afptest -c "afppasswd -c -p $VER2 -w $PASS" \
    || fail "create after an aborted attempt failed without -f"
pass "aborted create on a fresh directory leaves nothing behind"

echo "ALL afppasswd tests passed"
