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

# Exit immeditaly on error
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

# Validate MySQL/MariaDB CNID backend configuration
if [ "$AFP_CNID_BACKEND" = "mysql" ]; then
    if [ -z $AFP_CNID_SQL_HOST ]; then
        AFP_CNID_SQL_HOST="localhost"
    fi
    if [ -z $AFP_CNID_SQL_USER ]; then
        AFP_CNID_SQL_USER="netatalk"
    fi
    if [ -z $AFP_CNID_SQL_DB ]; then
        AFP_CNID_SQL_DB="cnid"
    fi

    echo "*** MySQL CNID backend configured: host='$AFP_CNID_SQL_HOST', user='$AFP_CNID_SQL_USER', db='$AFP_CNID_SQL_DB'"
    # Start MariaDB server if using localhost or 127.0.0.1 - for container-local database
    if [ "$AFP_CNID_SQL_HOST" = "localhost" ] || [ "$AFP_CNID_SQL_HOST" = "127.0.0.1" ]; then
        echo "*** Starting MariaDB server for MySQL CNID backend"
        mkdir -p /run/mysqld
        chown mysql:mysql /run/mysqld
        if [ ! -d /var/lib/mysql/mysql ]; then
            echo "*** Initializing MariaDB data directory"
            mysql_install_db --user=mysql --datadir=/var/lib/mysql > /dev/null 2>&1
        fi
        # Configure MariaDB to listen on TCP 127.0.0.1 in addition to Unix socket
        cat > /etc/my.cnf.d/netatalk.cnf << 'MYCNF'
[mysqld]
bind-address = 127.0.0.1
skip-networking = 0
MYCNF
        # Start MariaDB with TCP enabled
        mariadbd-safe --user=mysql &
        # Wait for MariaDB to be ready
        echo "*** Waiting for MariaDB to start..."
        for i in 1 2 3 4 5 6 7 8 9 10; do
            if mysqladmin ping > /dev/null 2>&1; then
                echo "*** MariaDB is ready"
                break
            fi
            sleep 1
        done
        # Configure MySQL user with password authentication
        # (MariaDB uses unix_socket auth by default, which only works when running as root)
        # Create user for both 'localhost' (Unix socket) and '127.0.0.1' (TCP)
        echo "*** Configuring MySQL user with password authentication"
        if [ -z "$AFP_CNID_SQL_PASS" ]; then
            AFP_CNID_SQL_PASS="netatalk"
        fi
        mariadb -u root << EOSQL
CREATE USER IF NOT EXISTS '$AFP_CNID_SQL_USER'@'localhost' IDENTIFIED BY '$AFP_CNID_SQL_PASS';
GRANT ALL PRIVILEGES ON *.* TO '$AFP_CNID_SQL_USER'@'localhost';
CREATE USER IF NOT EXISTS '$AFP_CNID_SQL_USER'@'127.0.0.1' IDENTIFIED BY '$AFP_CNID_SQL_PASS';
GRANT ALL PRIVILEGES ON *.* TO '$AFP_CNID_SQL_USER'@'127.0.0.1';
FLUSH PRIVILEGES;
EOSQL
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
dircachesize = ${AFP_DIRCACHESIZE:-8192}
dircache files = ${AFP_DIRCACHE_FILES:-no}
dircache validation freq = ${AFP_DIRCACHE_VALIDATION_FREQ:-1}
dircache metadata window = ${AFP_DIRCACHE_METADATA_WINDOW:-300}
dircache metadata threshold = ${AFP_DIRCACHE_METADATA_THRESHOLD:-60}
legacy icon = $AFP_LEGACY_ICON
log file = /var/log/afpd.log
log level = default:${AFP_LOGLEVEL:-info}
login message = $AFP_LOGIN_MESSAGE
mimic model = $AFP_MIMIC_MODEL
server name = ${SERVER_NAME:-Netatalk File Server}
uam list = $UAMS
mac charset = ${AFP_MAC_CHARSET:-MAC_ROMAN}
unix charset = ${AFP_UNIX_CHARSET:-UTF8}
vol charset = ${AFP_VOL_CHARSET:-UTF8}
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

if [ -z "$AFP_VERSION" ]; then
    AFP_VERSION=7
fi

if [ "$TESTSUITE" = "lan" ]; then
    if [ -n "$IO_MONITORING" ]; then
        if mkdir -p /proc_io && mount -t proc -o hidepid=0,gid=0 proc /proc_io 2> /dev/null; then
            echo "Successfully created /proc_io with hidepid=0,gid=0 for I/O monitoring (gid=0 as TESTSUITE tests run as root)"
        else
            echo "WARNING: IO_MONITORING enabled, however failed to create /proc_io mount"
            echo "NOTE: Container must be run with '--privileged' argument to allow proc mounts for afp_lantest IO monitoring"
        fi
    else
        echo "NOTE: TESTSUITE set to 'lan' for afp_lantest test, however IO_MONITORING=1 envvar not set (mounts /proc_io filesystem). IO monitoring disabled."
    fi
fi

echo "*** Starting AFP server"
if [ -z "$TESTSUITE" ]; then
    netatalk -d
else
    netatalk
    sleep 2
    echo "*** Running testsuite: $TESTSUITE"
    echo ""
    # Temporarily disable exit-on-error to ensure we can dump logs even if test fails
    set +e
    set -x
    TEST_EXIT_CODE=0
    case "$TESTSUITE" in
        spectest)
            afp_spectest $TEST_FLAGS -"$AFP_VERSION" -h "$AFP_HOST" -p "$AFP_PORT" -u "$AFP_USER" -d "$AFP_USER2" -w "$AFP_PASS" -s "$SHARE_NAME" -S "$SHARE_NAME2"
            TEST_EXIT_CODE=$?
            ;;
        readonly)
            echo "testfile uno" > /mnt/afpshare/first.txt
            echo "testfile dos" > /mnt/afpshare/second.txt
            mkdir /mnt/afpshare/third
            afp_spectest $TEST_FLAGS -"$AFP_VERSION" -h "$AFP_HOST" -p "$AFP_PORT" -u "$AFP_USER" -w "$AFP_PASS" -s "$SHARE_NAME" -f Readonly_test
            TEST_EXIT_CODE=$?
            ;;
        login)
            afp_logintest $TEST_FLAGS -"$AFP_VERSION" -h "$AFP_HOST" -p "$AFP_PORT" -u "$AFP_USER" -w "$AFP_PASS"
            TEST_EXIT_CODE=$?
            ;;
        lan)
            afp_lantest $TEST_FLAGS -"$AFP_VERSION" -h "$AFP_HOST" -p "$AFP_PORT" -u "$AFP_USER" -w "$AFP_PASS" -s "$SHARE_NAME"
            TEST_EXIT_CODE=$?
            ;;
        speed)
            afp_speedtest $TEST_FLAGS -"$AFP_VERSION" -h "$AFP_HOST" -p "$AFP_PORT" -u "$AFP_USER" -w "$AFP_PASS" -s "$SHARE_NAME" -n 5 -f Read,Write,Copy,ServerCopy
            TEST_EXIT_CODE=$?
            ;;
        *)
            echo "Unknown testsuite: $TESTSUITE"
            TEST_EXIT_CODE=1
            ;;
    esac
    set +x
    set -e

    echo "==== TESTS (${TESTSUITE}) COMPLETED (exit code: $TEST_EXIT_CODE) ===="
    # Display Netatalk's server logs if SERVER_LOGS environment variable is set
    if [ -n "$SERVER_LOGS" ]; then
        if [ -f /var/log/afpd.log ]; then
            echo "/var/log/afpd.log log lines: $(wc -l /var/log/afpd.log | awk '{print $1}')"
            echo "==== AFPD LOG CONTENT ===="
            cat /var/log/afpd.log
            echo "==== AFPD LOG END ===="
        else
            echo "NOTE: /var/log/afpd.log does not exist"
        fi
    fi
    exit $TEST_EXIT_CODE
fi
