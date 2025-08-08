#!/bin/sh

# Entry point script for the netatalk container.
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
    DISTRO=$(grep -E '^ID=' /etc/os-release | cut -d= -f2 | tr -d '"')
else
    echo "WARNING: /etc/os-release not found; unable to detect Linux distro"
fi

echo "*** Setting up users and groups"

if [ -z "$AFP_USER" ]; then
    echo "ERROR: AFP_USER needs to be set to use this container."
    exit 1
fi
if [ -z "$AFP_PASS" ]; then
    echo "ERROR: AFP_PASS needs to be set to use this container."
    exit 1
fi
if [ -z "$AFP_GROUP" ]; then
    echo "ERROR: AFP_GROUP needs to be set to use this container."
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
    adduser $uidcmd --no-create-home --disabled-password "$AFP_USER" 2> /dev/null || true
    addgroup $gidcmd "$AFP_GROUP" 2> /dev/null || true
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

echo "$AFP_USER:$AFP_PASS" | chpasswd > /dev/null 2>&1

if [ -f "/etc/netatalk/afppasswd" ]; then
    rm -f /etc/netatalk/afppasswd
fi

# Creating credentials for the RandNum UAM
afppasswd -c
afppasswd -a -f -w "$AFP_PASS" "$AFP_USER" > /dev/null

# Optional second user
if [ -n "$AFP_DROPBOX" ]; then
    if [ "$DISTRO" = "alpine" ]; then
        addgroup nobody "$AFP_GROUP"
    else
        usermod -aG $AFP_GROUP nobody 2> /dev/null || true
    fi
elif [ -n "$AFP_USER2" ]; then
    if [ "$DISTRO" = "alpine" ]; then
        adduser --no-create-home --disabled-password "$AFP_USER2" 2> /dev/null || true
        addgroup "$AFP_USER2" "$AFP_GROUP"
    else
        adduser --no-create-home --disabled-password --gecos '' "$AFP_USER2" 2> /dev/null || true
        usermod -aG $AFP_GROUP $AFP_USER2 2> /dev/null || true
    fi
    echo "$AFP_USER2:$AFP_PASS2" | chpasswd > /dev/null 2>&1
    afppasswd -a -f -w "$AFP_PASS2" "$AFP_USER2" > /dev/null
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

if [ -n "$AFP_DROPBOX" ] && [ -z "$SHARE_NAME2" ]; then
    SHARE_NAME2="Dropbox"
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

[ -n "$VERBOSE" ] && TEST_FLAGS="$TEST_FLAGS -v"

if [ -z "$AFP_HOST" ]; then
    AFP_HOST="127.0.0.1"
fi

if [ -z "$AFP_PORT" ]; then
    AFP_PORT="548"
fi

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

if [ -n "$ATALKD_INTERFACE" ]; then
    AFP_DDP="yes"
else
    AFP_DDP="no"
fi

if [ -n "$AFP_DROPBOX" ]; then
    AFP_VALIDUSERS1="$AFP_USER"
    AFP_VALIDUSERS2="$AFP_USER nobody"
else
    AFP_VALIDUSERS1="$AFP_USER $AFP_USER2"
    AFP_VALIDUSERS2="$AFP_USER $AFP_USER2"
fi

if [ "$AFP_CNID_BACKEND" = "mysql" ]; then
    if [ -z $AFP_CNID_SQL_HOST ]; then
        AFP_CNID_SQL_HOST="localhost"
    fi
    if [ -z $AFP_CNID_SQL_USER ]; then
        AFP_CNID_SQL_USER="root"
    fi
    if [ -z $AFP_CNID_SQL_DB ]; then
        AFP_CNID_SQL_DB="cnid"
    fi
fi

if [ "$TESTSUITE" = "spectest" ] && [ -z "$AFP_REMOTE" ]; then
    TEST_FLAGS="$TEST_FLAGS -c /mnt/afpshare"
fi

if [ -n "$AFP_CONFIG_POLLING" ]; then
    echo "*** Starting config file polling"
    /config_watch.sh /etc/netatalk/afp.conf "$AFP_CONFIG_POLLING" &
fi

if [ -z "$MANUAL_CONFIG" ]; then
    cat << EOF > /etc/netatalk/afp.conf
[Global]
appletalk = $AFP_DDP
cnid mysql host = $AFP_CNID_SQL_HOST
cnid mysql user = $AFP_CNID_SQL_USER
cnid mysql pw = $AFP_CNID_SQL_PASS
cnid mysql db = $AFP_CNID_SQL_DB
legacy icon = $AFP_LEGACY_ICON
log file = /var/log/afpd.log
log level = default:${AFP_LOGLEVEL:-info}
mimic model = $AFP_MIMIC_MODEL
server name = ${SERVER_NAME:-Netatalk File Server}
uam list = $UAMS
[${SHARE_NAME:-File Sharing}]
cnid scheme = ${AFP_CNID_BACKEND:-dbd}
ea = $AFP_EA
path = /mnt/afpshare
valid users = $AFP_VALIDUSERS1
volume name = ${SHARE_NAME:-File Sharing}
$AFP_RWRO = $AFP_VALIDUSERS1
[${SHARE_NAME2:-Time Machine}]
cnid scheme = ${AFP_CNID_BACKEND:-dbd}
ea = $AFP_EA
path = /mnt/afpbackup
time machine = $TIMEMACHINE
valid users = $AFP_VALIDUSERS2
volume name = ${SHARE_NAME2:-Time Machine}
$AFP_RWRO = $AFP_VALIDUSERS2
EOF
fi

if [ -n "$AFP_EXTMAP" ]; then
    sed -i 's/^#\./\./' /etc/netatalk/extmap.conf
fi

# Configuring AppleTalk if enabled
if [ -n "$ATALKD_INTERFACE" ]; then
    echo "*** Configuring DDP services"
    echo "$ATALKD_INTERFACE $ATALKD_OPTIONS" > /etc/netatalk/atalkd.conf
    echo "cupsautoadd:op=root:" > /etc/netatalk/papd.conf
    echo "*** Starting DDP services (this will take a minute)"
    cupsd
    atalkd
    papd
    timelord -l
    a2boot
else
    echo "NOTE: Set the \`ATALKD_INTERFACE' environment variable to start DDP services."
fi

echo "*** Starting AFP server"
if [ -z "$TESTSUITE" ]; then
    netatalk -d
else
    netatalk
    sleep 2
    echo "*** Running testsuite: $TESTSUITE"
    case "$TESTSUITE" in
        spectest)
            set -x
            afp_spectest $TEST_FLAGS -"$AFP_VERSION" -h "$AFP_HOST" -p "$AFP_PORT" -u "$AFP_USER" -d "$AFP_USER2" -w "$AFP_PASS" -s "$SHARE_NAME" -S "$SHARE_NAME2"
            ;;
        readonly)
            echo "testfile uno" > /mnt/afpshare/first.txt
            echo "testfile dos" > /mnt/afpshare/second.txt
            mkdir /mnt/afpshare/third
            set -x
            afp_spectest $TEST_FLAGS -"$AFP_VERSION" -h "$AFP_HOST" -p "$AFP_PORT" -u "$AFP_USER" -w "$AFP_PASS" -s "$SHARE_NAME" -f Readonly_test
            ;;
        login)
            set -x
            afp_logintest $TEST_FLAGS -"$AFP_VERSION" -h "$AFP_HOST" -p "$AFP_PORT" -u "$AFP_USER" -w "$AFP_PASS"
            ;;
        lan)
            set -x
            afp_lantest $TEST_FLAGS -"$AFP_VERSION" -h "$AFP_HOST" -p "$AFP_PORT" -u "$AFP_USER" -w "$AFP_PASS" -s "$SHARE_NAME"
            ;;
        speed)
            set -x
            afp_speedtest $TEST_FLAGS -"$AFP_VERSION" -h "$AFP_HOST" -p "$AFP_PORT" -u "$AFP_USER" -w "$AFP_PASS" -s "$SHARE_NAME" -n 5 -f Read,Write,Copy,ServerCopy
            ;;
        *)
            echo "Unknown testsuite: $TESTSUITE"
            exit 1
            ;;
    esac
fi
