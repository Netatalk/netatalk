#!/bin/sh

# Entry point script for netatalk docker container.
# Copyright (C) 2023  Eric Harmon
# Copyright (C) 2024-2025  Daniel Markstedt <daniel@mindani.net>
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

echo "*** Setting up environment"

set -e

[ -n "$DEBUG_ENTRY_SCRIPT" ] && set -x

DISTRO="unknown"
if [ -f /etc/os-release ]; then
    DISTRO=`grep -E '^ID=' /etc/os-release | cut -d= -f2 | tr -d '"'`
    echo "Detected Linux distribution: $DISTRO"
else
    echo "WARNING: /etc/os-release not found; are we running on Linux?"
fi

echo "*** Setting up users and groups"

if [ -z "$AFP_USER" ]; then
    echo "ERROR: AFP_USER needs to be set to use this Docker container."
    exit 1
fi
if [ -z "$AFP_PASS" ]; then
    echo "ERROR: AFP_PASS needs to be set to use this Docker container."
    exit 1
fi
if [ -z "$AFP_GROUP" ]; then
    echo "ERROR: AFP_GROUP needs to be set to use this Docker container."
    exit 1
fi


if [ "$DISTRO" = "alpine" ]; then
    uidcmd=""
    gidcmd=""
    if [ -n "$AFP_UID" ]; then
        uidcmd="-u $AFP_UID"
    fi
    if [ -n "$AFP_GID" ]; then
        gidcmd="-g $AFP_GID"
    fi
    adduser $uidcmd --no-create-home --disabled-password "$AFP_USER" 2>/dev/null || true
    addgroup $gidcmd "$AFP_GROUP" 2>/dev/null || true
    addgroup "$AFP_USER" "$AFP_GROUP"
else
    cmd=""
    if [ ! -z "$AFP_UID" ]; then
        cmd="$cmd --uid $AFP_UID"
    fi
    if [ ! -z "$AFP_GID" ]; then
        cmd="$cmd --gid $AFP_GID"
        groupadd --gid $AFP_GID $AFP_USER 2> /dev/null || true
    fi
    adduser $cmd --no-create-home --disabled-password --gecos '' "$AFP_USER" 2> /dev/null || true
    groupadd $AFP_GROUP 2> /dev/null || true
    usermod -aG $AFP_GROUP $AFP_USER 2> /dev/null || true
fi

echo "$AFP_USER:$AFP_PASS" | chpasswd

# Creating credentials for the RandNum UAM
if [ -f "/usr/local/etc/afppasswd" ]; then
    rm -f /usr/local/etc/afppasswd
fi
afppasswd -c
if ! afppasswd -a -f -w "$AFP_PASS" "$AFP_USER"; then
    echo "NOTE: Use a password of 8 chars or less to authenticate with Mac OS 8 or earlier clients"
fi

# Optional second user
if [ -n "$AFP_DROPBOX" ]; then
    if [ "$DISTRO" = "alpine" ]; then
        addgroup nobody "$AFP_GROUP"
    else
        usermod -aG $AFP_GROUP nobody 2> /dev/null || true
    fi
elif [ -n "$AFP_USER2" ]; then
    if [ "$DISTRO" = "alpine" ]; then
        adduser --no-create-home --disabled-password "$AFP_USER2" 2>/dev/null || true
        addgroup "$AFP_USER2" "$AFP_GROUP"
    else
        adduser --no-create-home --disabled-password --gecos '' "$AFP_USER2" 2> /dev/null || true
        usermod -aG $AFP_GROUP $AFP_USER2 2> /dev/null || true
    fi
    echo "$AFP_USER2:$AFP_PASS2" | chpasswd
    if ! afppasswd -a -f -w "$AFP_PASS2" "$AFP_USER2"; then
        echo "NOTE: Use a password of 8 chars or less to authenticate with Mac OS 8 or earlier clients"
    fi
fi

echo "*** Configuring shared volume"
[ -d /mnt/afpshare ] || mkdir /mnt/afpshare
[ -d /mnt/afpbackup ] || mkdir /mnt/afpbackup

echo "*** Fixing permissions"
if [ -n "$AFP_DROPBOX" ]; then
    chmod 2755 /mnt/afpshare
    chmod 2775 /mnt/afpbackup
else
    chmod 2775 /mnt/afpshare /mnt/afpbackup
fi
if [ -n "$AFP_UID" ] && [ -n "$AFP_GID" ]; then
    chown "$AFP_UID:$AFP_GID" /mnt/afpshare /mnt/afpbackup
else
    chown "$AFP_USER:$AFP_GROUP" /mnt/afpshare /mnt/afpbackup
fi

if [ -n "$AFP_DROPBOX" ] && [ -z "$SHARE2_NAME" ]; then
    SHARE2_NAME="Dropbox"
fi

echo "*** Removing residual lock files"

if [ "$DISTRO" = "alpine" ]; then
    mkdir -p /run/lock
    rm -f /run/lock/netatalk /run/lock/atalkd /run/lock/papd
else
    rm -f /var/lock/netatalk /var/lock/atalkd /var/lock/papd
fi

echo "*** Configuring Netatalk"
UAMS="uams_dhx.so uams_dhx2.so uams_randnum.so"

ATALK_NAME="${SERVER_NAME:-$(hostname | cut -d. -f1)}"
TEST_FLAGS=""
[ -n "$VERBOSE" ] && TEST_FLAGS="$TEST_FLAGS -v"

if [ -n "$INSECURE_AUTH" ] || [ -n "$AFP_DROPBOX" ]; then
    UAMS="$UAMS uams_clrtxt.so uams_guest.so"
fi

if [ -n "$DISABLE_TIMEMACHINE" ] || [ -n "$AFP_DROPBOX" ]; then
    TIMEMACHINE="no"
else
    TIMEMACHINE="yes"
fi

if [ -n "$AFP_READONLY" ]; then
    AFP_RWRO="rolist"
else
    AFP_RWRO="rwlist"
fi

if [ -n "$AFP_ADOUBLE" ]; then
    AFP_EA="ad"
    TEST_FLAGS="$TEST_FLAGS -a"
else
    AFP_EA="sys"
fi

if [ -n "$AFP_DROPBOX" ]; then
    AFP_VALIDUSERS1="$AFP_USER"
    AFP_VALIDUSERS2="$AFP_USER nobody"
else
    AFP_VALIDUSERS1="$AFP_USER $AFP_USER2"
    AFP_VALIDUSERS2="$AFP_USER $AFP_USER2"
fi

if [ -z "$MANUAL_CONFIG" ]; then
    cat <<EOF > /usr/local/etc/afp.conf
[Global]
appletalk = yes
legacy icon = $AFP_LEGACY_ICON
log file = /var/log/afpd.log
log level = default:${AFP_LOGLEVEL:-info}
mimic model = $AFP_MIMIC_MODEL
server name = ${SERVER_NAME:-Netatalk File Server}
spotlight = yes
uam list = $UAMS
[${SHARE_NAME:-File Sharing}]
ea = $AFP_EA
path = /mnt/afpshare
valid users = $AFP_VALIDUSERS1
volume name = ${SHARE_NAME:-File Sharing}
$AFP_RWRO = $AFP_VALIDUSERS1
[${SHARE2_NAME:-Time Machine}]
ea = $AFP_EA
path = /mnt/afpbackup
time machine = $TIMEMACHINE
valid users = $AFP_VALIDUSERS2
volume name = ${SHARE2_NAME:-Time Machine}
$AFP_RWRO = $AFP_VALIDUSERS2
EOF
fi

# Configuring AppleTalk if enabled
if [ -n "$ATALKD_INTERFACE" ]; then
    echo "*** Configuring DDP services"
    echo "$ATALKD_INTERFACE $ATALKD_OPTIONS" > /usr/local/etc/atalkd.conf
    echo "cupsautoadd:op=root:" > /usr/local/etc/papd.conf
    echo "*** Starting DDP services (this will take a minute)"
    cupsd
    atalkd
    papd
    timelord -l
    a2boot
else
    echo "Set the \`ATALKD_INTERFACE' environment variable to start DDP services."
fi

echo "*** Starting AFP server"
if [ -z "$TESTSUITE" ]; then
    if [ -z "$AFP_DRYRUN" ]; then
        netatalk -d
    else
        netatalk -V
    fi
else
    if [ "$TESTSUITE" = "spectest" ]; then
    cat <<EXT > /usr/local/etc/extmap.conf
.         "????"  "????"      Unix Binary                    Unix                      application/octet-stream
.doc      "WDBN"  "MSWD"      Word Document                  Microsoft Word            application/msword
.pdf      "PDF "  "CARO"      Portable Document Format       Acrobat Reader            application/pdf
EXT
    fi
    netatalk
    sleep 2
    case "$TESTSUITE" in
        spectest)
            afp_spectest -"$AFP_VERSION" -V -C -h 127.0.0.1 -p 548 -u "$AFP_USER" -d "$AFP_USER2" -w "$AFP_PASS" -s "$SHARE_NAME" -S "$SHARE2_NAME"
            ;;
        readonly)
            echo "testfile uno" > /mnt/afpshare/first.txt
            echo "testfile dos" > /mnt/afpshare/second.txt
            mkdir /mnt/afpshare/third
            afp_spectest $TEST_FLAGS -"$AFP_VERSION" -h 127.0.0.1 -p 548 -u "$AFP_USER" -w "$AFP_PASS" -s "$SHARE_NAME" -f Readonly_test
            ;;
        login)
            afp_logintest $TEST_FLAGS -"$AFP_VERSION" -h 127.0.0.1 -p 548 -u "$AFP_USER" -w "$AFP_PASS"
            ;;
        lan)
            afp_lantest $TEST_FLAGS -"$AFP_VERSION" -h 127.0.0.1 -p 548 -u "$AFP_USER" -w "$AFP_PASS" -s "$SHARE_NAME"
            ;;
        speed)
            afp_speedtest $TEST_FLAGS -"$AFP_VERSION" -h 127.0.0.1 -p 548 -u "$AFP_USER" -w "$AFP_PASS" -s "$SHARE_NAME"
            ;;
        *)
            echo "Unknown testsuite: $TESTSUITE"
            exit 1
            ;;
    esac
fi
