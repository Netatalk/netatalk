#!/bin/sh
# Private volume integration tests.
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

# passwords a cracklib-enabled afppasswd accepts
PASS1=Kw7vTq9zLp2rXm4b
PASS2=Jx3nRd8vPq5tYw7c
LOG=/var/log/afpd.log
CONF=$(netatalk_confdir)/afp.conf
PRIV=alicevol
PRIV_DIR=/srv/alice
PRIV_DB=$PRIV_DIR/.AppleDB
HOME_DIR=/home/alice
HOME_VOL="alice's home"
HOME_DB=$HOME_DIR/.AppleDB
NAD_OUT=/tmp/pv_nad
REFUSED="a private volume can't be used with"
# the controller's list of volumes for the localsearch indexer
INDEXER_LIST=/etc/dconf/db/netatalk.d/10-spotlight

trap 'stop_netatalk ""' EXIT
stop_netatalk ""

for u in alice bob; do
    id $u > /dev/null 2>&1 || adduser -D $u
done

CHOWN_FIX="fix with: chown -R $(id -u alice) "
mkdir -p /srv/shared $PRIV_DIR /srv/ghost /srv/rooty /srv/dollar \
    "$(dirname "$CONF")" /var/log
chown alice $PRIV_DIR
chmod 777 /srv/shared
rm -rf "$(netatalk_confdir)/afppasswd.srp"
afppasswd -c
afppasswd -a alice -w $PASS1 > /dev/null
afppasswd -a bob -w $PASS2 > /dev/null

# The configuration every leg starts from; a leg adds its own lines after it.
write_conf() {
    cat > "$CONF" << EOF
[Global]
uam list = uams_srp.so
log file = $LOG

[Homes]
basedir regex = /home

[shared]
path = /srv/shared

[$PRIV]
path = $PRIV_DIR
private = alice

[ghost]
path = /srv/ghost
private = nosuchuser

[rooty]
path = /srv/rooty
private = root

[dollar]
path = /srv/dollar
private = \$u
EOF
    return 0
}

# Write the configuration and serve it: each pair of arguments is a section
# name and a line added under it. Volume options are read when a volume is
# loaded, so the server restarts, with a fresh log.
serve_conf() {
    write_conf

    while [ $# -ge 2 ]; do
        sc_section=$1
        sc_line=$2
        sed -i "/^\[$sc_section\]/a $sc_line" "$CONF"
        shift 2
    done

    stop_netatalk ""
    : > $LOG
    start_root_netatalk
    return 0
}

# $1 is a CNID directory that must be alice's owner-only state.
alice_state() {
    as_dir=$1
    owner_only_cnid "$as_dir" alice \
        || fail "$as_dir is not alice's owner-only CNID state"
    return 0
}

serve_conf

# --- a section whose owner can't be resolved or is root, or that names $u,
# is refused when the file is loaded
grep -q "\"ghost\": 'private' user \"nosuchuser\" is unknown" $LOG \
    || fail "an unknown private user was not refused"
grep -q "\"rooty\": root can't own a private volume" $LOG \
    || fail "a private volume owned by root was not refused"
grep -q "\"dollar\": 'private' user \"\$u\" is unknown" $LOG \
    || fail "'private = \$u' in a volume section was not refused"
afp_can_open alice $PASS1 ghost && fail "the volume of an unknown user was served"
afp_can_open alice $PASS1 dollar && fail "a 'private = \$u' volume section was served"
pass "unknown and root owners, and \$u, are refused at load"

# --- the volume is its owner's alone, and keeps its state inside it
afp_can_open bob $PASS2 $PRIV && fail "bob opened alice's private volume"
afp_can_open alice $PASS1 $PRIV || fail "alice could not open her private volume"
afp_can_open bob $PASS2 shared || fail "bob could not open the shared volume"
alice_state $PRIV_DB
grep -q "is private to" $LOG && fail "turning bob away was logged at the default level"
pass "a private volume is served to its owner only, with its state inside it"

# --- nad run by the owner opens it; run by root it opens the state as the
# owner, so what it creates is alice's; run by bob it does not get the volume
su alice -c "nad -F $CONF ls $PRIV_DIR" > /dev/null \
    || fail "alice's nad could not open her private volume"
rm -rf $PRIV_DB
nad -F "$CONF" ls $PRIV_DIR > /dev/null || fail "root's nad could not open the private volume"
alice_state $PRIV_DB
su bob -c "nad -F $CONF ls $PRIV_DIR" > $NAD_OUT 2>&1 \
    && fail "bob's nad opened alice's private volume"
grep -q "\"$PRIV_DIR\" is not inside an AFP volume" $NAD_OUT \
    || fail "bob's nad was given alice's private volume"
pass "root's nad opens the state as the owner, and another account's nad gets no volume"

# --- bob's nad reaches alice's home through [Homes], which is built with
# alice's record, but the load is bob's, so it does not get the volume. The
# home is opened to everyone for this leg, so admission is what keeps bob from
# creating state there.
HOME_MODE=$(stat -c %a $HOME_DIR)
chmod 777 $HOME_DIR
nad_rc=0
su bob -c "nad -F $CONF ls $HOME_DIR" > $NAD_OUT 2>&1 || nad_rc=$?
chmod "$HOME_MODE" $HOME_DIR
[ "$nad_rc" -ne 0 ] || fail "bob's nad opened alice's home"
grep -q "\"$HOME_DIR\" is not inside an AFP volume" $NAD_OUT \
    || fail "bob's nad was given alice's home"
[ ! -e $HOME_DB ] || fail "bob's nad created state in alice's home"
pass "another account's nad does not get alice's home"

# --- every home is private to its owner, with its state inside it
afp_can_open alice $PASS1 "$HOME_VOL" || fail "alice could not open her home"
alice_state $HOME_DB
pass "every home is private to its owner, with its state inside it"

# --- dbd run by root creates a home's state as the home's owner
stop_netatalk ""
rm -rf $HOME_DB
dbd -F "$CONF" -s $HOME_DIR > /dev/null 2>&1 || fail "dbd could not scan alice's home"
alice_state $HOME_DB
pass "dbd run by root creates a home's state as its owner"

# --- a server that keeps its shared volumes on mysql and searches them with
# localsearch serves private volumes, a home included, on their own sqlite
# databases, and leaves them out of the indexer's list
rm -rf $PRIV_DB $HOME_DB
serve_conf Global 'cnid scheme = mysql' Global 'spotlight backend = localsearch'
afp_can_open alice $PASS1 $PRIV \
    || fail "a private volume did not open on a mysql and localsearch server"
alice_state $PRIV_DB
afp_can_open alice $PASS1 "$HOME_VOL" \
    || fail "a home did not open on a mysql and localsearch server"
alice_state $HOME_DB
grep -q "doesn't apply to a private volume" $LOG \
    && fail "a [Global] cnid scheme or spotlight backend was logged against a private volume"

# the controller writes the list after afpd is up, so it is waited for
case "$(afpd_buildinfo 'Spotlight search backends')" in
    *localsearch*)
        wait_for 10 grep -q "'/srv/shared'" $INDEXER_LIST \
            || fail "the shared volume is not in the indexer's list"
        grep -q "'$PRIV_DIR'" $INDEXER_LIST \
            && fail "the private volume is in the indexer's list"
        ;;
    *)
        echo "# indexer list not checked: this build has no localsearch backend"
        ;;
esac

pass "private volumes keep their own sqlite state on a mysql and localsearch server"

# --- turning an account away is logged at info
serve_conf Global 'log level = default:info'
afp_can_open bob $PASS2 $PRIV && fail "bob opened alice's private volume"
grep -q "volume \"$PRIV\" is private to \"alice\", skipping" $LOG \
    || fail "turning bob away was not logged at info"
pass "turning an account away is logged at info"

# --- the global vol dbpath moves shared state and leaves private state inside
# the volume
mkdir -p /srv/globalcnid
rm -rf $PRIV_DB
serve_conf Global 'vol dbpath = /srv/globalcnid'
afp_can_open alice $PASS1 $PRIV || fail "a global vol dbpath broke alice's volume"
alice_state $PRIV_DB
afp_can_open bob $PASS2 shared || fail "a global vol dbpath broke the shared volume"
ls /srv/globalcnid/shared/*.sqlite > /dev/null \
    || fail "the shared volume's state is not under the global vol dbpath"
pass "the global vol dbpath does not move private state"

# --- a vol dbpath set for the volume moves its state, and the directory is
# made owner-only
mkdir -p /srv/alicecnid
chown alice /srv/alicecnid
chmod 755 /srv/alicecnid
serve_conf $PRIV 'vol dbpath = /srv/alicecnid'
afp_can_open alice $PASS1 $PRIV || fail "a vol dbpath for the volume broke it"
alice_state /srv/alicecnid
pass "a vol dbpath for the volume moves its state and keeps it owner-only"

# --- a [Homes] vol dbpath names each owner with $u, under a directory the
# owners may create in; one without $u is passed over for the home's .AppleDB
mkdir -p /srv/homecnid
chmod 1777 /srv/homecnid
rm -rf $HOME_DB
serve_conf Homes 'vol dbpath = /srv/homecnid/$u'
afp_can_open alice $PASS1 "$HOME_VOL" || fail "a home did not open with a \$u vol dbpath"
alice_state /srv/homecnid/alice
[ ! -e $HOME_DB ] || fail "a \$u vol dbpath left state in the home"
serve_conf Homes 'vol dbpath = /srv/homecnid'
afp_can_open alice $PASS1 "$HOME_VOL" || fail "a home did not open on a vol dbpath without \$u"
alice_state $HOME_DB
grep -q "'vol dbpath' /srv/homecnid needs \$u" $LOG \
    || fail "a [Homes] vol dbpath without \$u was not logged"
pass "a [Homes] vol dbpath names each owner with \$u, or is passed over"

# --- force user makes every session act as one account: a private volume is
# skipped at the load, and a home when root's nad reaches it. A session of a
# non-admin account does not log in under force user at all, so nad is the
# load that builds a home here
serve_conf Global 'force user = bob'
grep -q "\"$PRIV\": $REFUSED force user" $LOG \
    || fail "a private volume under force user was not refused"
nad -F "$CONF" ls $HOME_DIR > /dev/null 2>&1 \
    && fail "root's nad was given a home under force user"
grep -q "\"$HOME_VOL\": $REFUSED force user" $LOG \
    || fail "a home under force user was not logged"
pass "force user skips private volumes and homes"

# --- the next open makes widened state owner-only
serve_conf
afp_can_open alice $PASS1 $PRIV || fail "alice's volume did not open"
chmod 777 $PRIV_DB
chmod 666 $PRIV_DB/*.sqlite
afp_can_open alice $PASS1 $PRIV || fail "alice's volume did not open over widened state"
alice_state $PRIV_DB
pass "the next open makes widened private state owner-only"

# --- state another account owns is refused with the chown that fixes it,
# owner-only, which refuses the open, and readable, which reaches the
# ownership check, and it is left as it was
chown -R bob $PRIV_DB
chmod 700 $PRIV_DB
: > $LOG
afp_can_open alice $PASS1 $PRIV && fail "alice's volume opened over bob's owner-only state"
grep -q "$CHOWN_FIX" $LOG || fail "bob's owner-only state did not name the chown that fixes it"
chmod 755 $PRIV_DB
: > $LOG
afp_can_open alice $PASS1 $PRIV && fail "alice's volume opened over bob's readable state"
grep -q "$CHOWN_FIX" $LOG || fail "bob's readable state did not name the chown that fixes it"
owned $PRIV_DB "bob 755" || fail "bob's state was taken over"
chown -R alice $PRIV_DB
pass "state another account owns is refused with the chown that fixes it"

# --- the desktop database is created by the owner inside the private state
afp_can_open alice $PASS1 $PRIV 548 FPOpenDT \
    || fail "FPOpenDT failed on alice's private volume"
[ "$(stat -c %U $PRIV_DB/.AppleDesktop 2> /dev/null)" = alice ] \
    || fail "the desktop database is not alice's"
alice_state $PRIV_DB
pass "the desktop database is the owner's, inside the owner-only CNID directory"

# --- a private CNID directory is never a symbolic link, also behind a
# trailing slash: the volume does not open, no chown is named, and the link's
# target is left as it was
mkdir -p /srv/bobcnid
chown bob /srv/bobcnid
chmod 700 /srv/bobcnid
ln -sfn /srv/bobcnid /srv/alicelink
serve_conf $PRIV 'vol dbpath = /srv/alicelink/'
afp_can_open alice $PASS1 $PRIV && fail "alice's volume opened through a symbolic link"
grep -q "fix with: chown" $LOG && fail "a symbolic link was answered with a chown"
grep -q "Can't open CNID DB directory '/srv/alicelink'" $LOG \
    || fail "the symbolic link was not refused as a CNID directory"
owned /srv/bobcnid "bob 700" || fail "the link's target was changed"
pass "a symbolic link is refused as a private CNID directory, trailing slash included"

echo "ALL private volume tests passed"
