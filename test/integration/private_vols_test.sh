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
PASS3=Vq8mZt3xNr6wKp2d
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

# carol's passwd record outgrows the buffer the loader starts with, 16 KiB
# where the C library names no size
id carol > /dev/null 2>&1 \
    || adduser -D -g "$(head -c 40000 /dev/zero | tr '\0' x)" carol

CHOWN_FIX="if it holds only this volume's state, fix with: chown -R $(id -u alice) "
mkdir -p /srv/shared $PRIV_DIR /srv/carol /srv/ghost /srv/rooty /srv/dollar \
    "$(dirname "$CONF")" /var/log
chown alice $PRIV_DIR
chown carol /srv/carol
chmod 777 /srv/shared
rm -rf "$(netatalk_confdir)/afppasswd.srp"
afppasswd -c
afppasswd -a alice -w $PASS1 > /dev/null
afppasswd -a bob -w $PASS2 > /dev/null
afppasswd -a carol -w $PASS3 > /dev/null

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

[carolvol]
path = /srv/carol
private = carol

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

# --- the owner's record is read whatever its length
afp_can_open carol $PASS3 carolvol \
    || fail "carol, whose passwd record outgrows the loader's buffer, did not get the private volume"
pass "a private volume opens for an owner whose passwd record outgrows the loader's buffer"

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

# --- a fresh shared CNID directory ends 01777 whatever the umask of the tool
# that creates it
SHARED_CNID="$(afpd_buildinfo 'state directory')CNID/shared"
rm -rf "$SHARED_CNID"
mkdir -p "$(dirname "$SHARED_CNID")"
(umask 022 && nad -F "$CONF" ls /srv/shared > /dev/null) \
    || fail "root's nad could not open the shared volume"
owned "$SHARED_CNID" "root 1777" || fail "a fresh shared CNID directory is not 01777"
pass "a fresh shared CNID directory is 01777 whatever the tool's umask"

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

# --- a CNID directory the owner cannot create is refused, naming the commands
# that create it for the owner, with the path quoted for the shell
mkdir -p /srv/rootonly
chmod 755 /srv/rootonly
serve_conf $PRIV 'vol dbpath = /srv/rootonly/alicecnid'
afp_can_open alice $PASS1 $PRIV && fail "alice's volume opened without its CNID directory"
grep -q "fix with: mkdir -p -m 700 '/srv/rootonly/alicecnid' && chown $(id -u alice) '/srv/rootonly/alicecnid'" $LOG \
    || fail "the failed creation did not name the commands that create the directory"
QUOTED="'/srv/rootonly/alice'\\''s cnid'"
serve_conf $PRIV "vol dbpath = /srv/rootonly/alice's cnid"
afp_can_open alice $PASS1 $PRIV && fail "alice's volume opened without its quoted CNID directory"
grep -qF "fix with: mkdir -p -m 700 $QUOTED && chown $(id -u alice) $QUOTED" $LOG \
    || fail "the commands for a path with a single quote did not quote it for the shell"
pass "a CNID directory the owner cannot create names the commands that create it"

# --- so does one under a missing parent, which the same commands create; a
# shared volume's, which root creates, names none
serve_conf $PRIV 'vol dbpath = /srv/nowhere/alicecnid' shared 'vol dbpath = /srv/nowhere/sharedcnid'
afp_can_open alice $PASS1 $PRIV && fail "alice's volume opened under a missing parent"
grep -q "fix with: mkdir -p -m 700 '/srv/nowhere/alicecnid' && chown $(id -u alice) '/srv/nowhere/alicecnid'" $LOG \
    || fail "the failed creation under a missing parent did not name the commands"
afp_can_open bob $PASS2 shared && fail "the shared volume opened under a missing parent"
grep -q "fix with: mkdir -p -m 700 '/srv/nowhere/sharedcnid" $LOG \
    && fail "a shared volume's failed creation named the private commands"
pass "a CNID directory under a missing parent names the commands that create it"

# --- a creation those commands do not cure names none: a file in the path
: > /srv/afile
serve_conf $PRIV 'vol dbpath = /srv/afile/cnid'
afp_can_open alice $PASS1 $PRIV && fail "alice's volume opened with a file in its CNID path"
grep -q "Failed to create CNID DB directory '/srv/afile/cnid'" $LOG \
    || fail "the failed creation under a file was not logged"
grep -q "fix with" $LOG && fail "a creation no command cures named one"
pass "a CNID directory no command can create names none"

# --- the volume's own root is refused as its CNID directory before the owner
# check, so a root another account owns draws no chown, and a root the owner
# could make owner-only keeps its mode
chown bob $PRIV_DIR
serve_conf $PRIV "vol dbpath = $PRIV_DIR"
afp_can_open alice $PASS1 $PRIV && fail "alice's volume opened on its own root as CNID directory"
grep -q "CNID DB directory '$PRIV_DIR' is the volume itself" $LOG \
    || fail "the volume's own root was not refused"
grep -q "fix with" $LOG && fail "the volume's own root was answered with a chown"
chown alice $PRIV_DIR
chmod 755 $PRIV_DIR
: > $LOG
opened=0
afp_can_open alice $PASS1 $PRIV || opened=$?
owned $PRIV_DIR "alice 755" || fail "the refused root changed mode"
[ $opened -ne 0 ] || fail "alice's volume opened on her own root as CNID directory"
grep -q "CNID DB directory '$PRIV_DIR' is the volume itself" $LOG \
    || fail "alice's own root was not refused"
pass "the volume's own root is refused as its CNID directory"

# --- a [Homes] vol dbpath names each owner with $u, a directory root makes
# for that owner; one without $u is passed over for the home's .AppleDB, and
# an empty one gives it unlogged
mkdir -p /srv/homecnid
install -d -m 700 -o alice /srv/homecnid/alice
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
rm -rf $HOME_DB
serve_conf Homes 'vol dbpath ='
afp_can_open alice $PASS1 "$HOME_VOL" || fail "a home did not open with an empty vol dbpath"
alice_state $HOME_DB
grep -q "needs \$u" $LOG && fail "an empty [Homes] vol dbpath was logged"
pass "a [Homes] vol dbpath names each owner with \$u, or is passed over"

# --- force user makes every session act as one account: a private volume is
# skipped at the load, and the homes are shared. A session of a non-admin
# account does not log in under force user at all, so nad is the load that
# builds a home here
rm -rf $HOME_DB
serve_conf Global 'force user = bob'
grep -q "\"$PRIV\": $REFUSED force user" $LOG \
    || fail "a private volume under force user was not refused"
nad -F "$CONF" ls $HOME_DIR > /dev/null \
    || fail "root's nad was not given a home under force user"
[ ! -e $HOME_DB ] || fail "a home under force user kept its state inside it"
pass "force user skips private volumes and shares the homes"

# --- '[Homes] private =' shares the homes; a name there is logged and the
# homes stay private
serve_conf Homes 'private ='
afp_can_open alice $PASS1 "$HOME_VOL" || fail "alice could not open her shared home"
[ ! -e $HOME_DB ] || fail "a shared home kept its state inside it"
serve_conf Homes 'private = bob'
afp_can_open alice $PASS1 "$HOME_VOL" || fail "alice could not open her home"
grep -q "\[Homes\] 'private' takes no name" $LOG \
    || fail "a name in [Homes] 'private' was not logged"
alice_state $HOME_DB
pass "'[Homes] private =' shares the homes, and a name there is passed over"

# --- the next open makes widened state owner-only
serve_conf
afp_can_open alice $PASS1 $PRIV || fail "alice's volume did not open"
chmod 777 $PRIV_DB
chmod 666 $PRIV_DB/*.sqlite
afp_can_open alice $PASS1 $PRIV || fail "alice's volume did not open over widened state"
alice_state $PRIV_DB
pass "the next open makes widened private state owner-only"

# --- state another account owns is refused with the chown that fixes it,
# owner-only, which refuses the open, and writable, which reaches the
# ownership check and is left as it was; the chown quotes the path for the
# shell
chown -R bob $PRIV_DB
chmod 700 $PRIV_DB
: > $LOG
afp_can_open alice $PASS1 $PRIV && fail "alice's volume opened over bob's owner-only state"
grep -q "$CHOWN_FIX" $LOG || fail "bob's owner-only state did not name the chown that fixes it"
chmod 777 $PRIV_DB
BOB_STATE=$(ls -A $PRIV_DB)
: > $LOG
opened=0
afp_can_open alice $PASS1 $PRIV || opened=$?
owned $PRIV_DB "bob 777" && [ "$(ls -A $PRIV_DB)" = "$BOB_STATE" ] \
    || fail "bob's state was taken over"
[ $opened -ne 0 ] || fail "alice's volume opened over bob's writable state"
grep -q "$CHOWN_FIX" $LOG || fail "bob's writable state did not name the chown that fixes it"
chown -R alice $PRIV_DB
mkdir -p "/srv/bob's state"
chown bob "/srv/bob's state"
serve_conf $PRIV "vol dbpath = /srv/bob's state"
afp_can_open alice $PASS1 $PRIV && fail "alice's volume opened over bob's state in a quoted path"
grep -qF "fix with: chown -R $(id -u alice) '/srv/bob'\\''s state'" $LOG \
    || fail "the chown for a path with a single quote did not quote it for the shell"
serve_conf
pass "state another account owns is refused with the chown that fixes it"

# --- the desktop database is created by the owner inside the private state
afp_can_open alice $PASS1 $PRIV 548 FPOpenDT \
    || fail "FPOpenDT failed on alice's private volume"
[ "$(stat -c %U $PRIV_DB/.AppleDesktop 2> /dev/null)" = alice ] \
    || fail "the desktop database is not alice's"
alice_state $PRIV_DB
pass "the desktop database is the owner's, inside the owner-only CNID directory"

# --- a .AppleDesktop the owner keeps in the volume root moves into the
# private state; one the owner cannot move stays, beside a fresh one
mkdir $PRIV_DIR/.AppleDesktop
: > $PRIV_DIR/.AppleDesktop/icons
chown -R alice $PRIV_DIR/.AppleDesktop
rm -rf $PRIV_DB/.AppleDesktop
afp_can_open alice $PASS1 $PRIV 548 FPOpenDT \
    || fail "FPOpenDT failed with alice's .AppleDesktop in the volume root"
[ -e $PRIV_DB/.AppleDesktop/icons ] && [ ! -e $PRIV_DIR/.AppleDesktop ] \
    || fail "alice's .AppleDesktop did not move into her CNID directory"
rm -rf $PRIV_DB/.AppleDesktop
install -d -m 755 $PRIV_DIR/.AppleDesktop
afp_can_open alice $PASS1 $PRIV 548 FPOpenDT \
    || fail "FPOpenDT failed with root's .AppleDesktop in the volume root"
[ -d $PRIV_DIR/.AppleDesktop ] \
    && [ "$(stat -c %U $PRIV_DB/.AppleDesktop 2> /dev/null)" = alice ] \
    || fail "root's .AppleDesktop was moved, or the fresh one is not alice's"
grep -q "moving .AppleDesktop from \"$PRIV_DIR/.AppleDesktop\" failed: Permission denied" $LOG \
    || fail "the .AppleDesktop alice cannot move was not reported with its cause"
rm -rf $PRIV_DIR/.AppleDesktop
pass "a .AppleDesktop in the volume root moves into the private state when the owner can move it"

# --- an admin-group session runs as root and creates the private state and
# the desktop database as the owner
addgroup afpadmins 2> /dev/null || true
addgroup alice afpadmins 2> /dev/null || true
id -nG alice | grep -qw afpadmins || fail "alice is not in afpadmins"
rm -rf $PRIV_DB
serve_conf Global 'admin group = afpadmins' Global 'log level = default:info'
afp_can_open alice $PASS1 $PRIV 548 FPOpenDT \
    || fail "FPOpenDT failed in alice's admin session"
grep -q "admin login -- alice" $LOG || fail "alice's session was not an admin login"
alice_state $PRIV_DB
[ "$(stat -c %U $PRIV_DB/.AppleDesktop 2> /dev/null)" = alice ] \
    || fail "the desktop database an admin session created is not alice's"
pass "an admin-group session creates the private state and the .AppleDesktop folder as the owner"

# --- root's nad takes the owner's groups too: alice's state is created in her
# group, under a directory that only her supplementary group afpadmins can write
install -d -m 770 -g afpadmins /srv/grpcnid
serve_conf $PRIV 'vol dbpath = /srv/grpcnid/alicecnid'
nad -F "$CONF" ls $PRIV_DIR > /dev/null \
    || fail "root's nad could not create alice's state under her group's directory"
alice_state /srv/grpcnid/alicecnid
[ "$(stat -c %G /srv/grpcnid/alicecnid)" = "$(id -gn alice)" ] \
    || fail "root's nad did not create the state in alice's group"
pass "root's nad creates the state with the owner's groups"

# --- a private CNID directory is never a symbolic link, also behind a
# trailing slash: the volume does not open, no chown is named, and a target
# the owner could change is left as it was
mkdir -p /srv/bobcnid
chown bob /srv/bobcnid
chmod 700 /srv/bobcnid
ln -sfn /srv/bobcnid /srv/alicelink
serve_conf $PRIV 'vol dbpath = /srv/alicelink/'
afp_can_open alice $PASS1 $PRIV && fail "alice's volume opened through a symbolic link"
grep -q "fix with: chown" $LOG && fail "a symbolic link was answered with a chown"
grep -q "CNID DB directory '/srv/alicelink' is a symbolic link" $LOG \
    || fail "the symbolic link was not refused as a CNID directory"
serve_conf $PRIV 'vol dbpath = /srv/alicelink/.'
afp_can_open alice $PASS1 $PRIV && fail "alice's volume opened through a symbolic link and a final ."
grep -q "CNID DB directory '/srv/alicelink/.' ends in . or .." $LOG \
    || fail "a CNID path ending in . was not refused"
mkdir -p /srv/alicetarget
chown alice /srv/alicetarget
chmod 755 /srv/alicetarget
ln -sfn /srv/alicetarget /srv/alicelink
serve_conf $PRIV 'vol dbpath = /srv/alicelink/'
opened=0
afp_can_open alice $PASS1 $PRIV || opened=$?
owned /srv/alicetarget "alice 755" && [ -z "$(ls -A /srv/alicetarget)" ] \
    || fail "the link's target was changed"
[ $opened -ne 0 ] || fail "alice's volume opened through a symbolic link to her own directory"
pass "a symbolic link is refused as a private CNID directory, trailing slash included"

# --- a bare .. is refused too: a session would open the directory above the
# volume root
serve_conf $PRIV 'vol dbpath = ..'
afp_can_open alice $PASS1 $PRIV && fail "alice's volume opened on the directory above its root"
grep -q "CNID DB directory '..' ends in . or .." $LOG \
    || fail "a bare .. was not refused as a CNID path"
pass "a CNID path of a bare .. is refused"

# --- a reload request makes the controller read afp.conf again, changed or
# not: each read logs the unknown owner of [ghost]
serve_conf
: > $LOG
pkill -HUP -x netatalk
wait_for 10 grep -q "netatalk\[[0-9]*\] .*\"ghost\": 'private' user" $LOG \
    || fail "the controller did not read an unchanged afp.conf again on a reload request"
pass "a reload request makes the controller read an unchanged afp.conf again"

# --- a volume renamed on its path is built at the same reload request: the
# controller's second read finds the path free and records the new name's uuid
VOLUUID_CONF=$(afpd_buildinfo afp_voluuid.conf)
[ -n "$VOLUUID_CONF" ] || fail "afpd -V does not report afp_voluuid.conf"
sed -i '/^\[shared\]/a volume name = sharedrenamed' "$CONF"
pkill -HUP -x netatalk
wait_for 10 grep -q '^"sharedrenamed"' "$VOLUUID_CONF" \
    || fail "the reload request did not build the volume renamed on its path"
pass "a reload request builds a volume renamed on its path"

echo "ALL private volume tests passed"
