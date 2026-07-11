#!/bin/sh

# Copyright (C) 2023  Eric Harmon
# Copyright (C) 2024-2026  Daniel Markstedt <daniel@mindani.net>
# Copyright (C) 2025-2026  Andy Lemin (andylemin)
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
# Common environment setup for netatalk containers.
# Sourced by entrypoint_netatalk.sh and debug_entrypoint_netatalk.sh.
#
# This script handles:
#   - Linux distro detection
#   - User / group creation
#   - AFP password provisioning
#   - Shared volume and permission setup
#   - Lock file cleanup
#   - Netatalk configuration variable preparation
#   - MySQL / MariaDB CNID backend bootstrap
#   - afp.conf generation (unless MANUAL_CONFIG is set)
#   - extmap.conf activation
#   - AppleTalk (DDP) service startup
#   - AFP protocol version default
#   - I/O monitoring proc mount for lantest

# --------------------------------------------------------------------------
# Detect Linux distribution
# --------------------------------------------------------------------------

DISTRO_ALPINE="alpine"

DISTRO="unknown"
if [ -f /etc/os-release ]; then
    DISTRO=$(grep -E '^ID=' /etc/os-release | cut -d= -f2 | tr -d '"')
else
    echo "WARNING: /etc/os-release not found; unable to detect Linux distro" >&2
fi

# Detect the OS kernel. This is orthogonal to the Linux distro detection
# above: Linux containers report "Linux" (so every existing code path is
# unchanged), while the *BSD / illumos CI VMs report their own kernel name
# and select the matching user-provisioning branches below.
OS_KERNEL=$(uname -s 2> /dev/null || echo Linux)

# Map the kernel to the user/group provisioning toolchain. Linux (every
# container image) keeps USER_TOOL empty, so the existing Alpine/glibc branches
# run unchanged. FreeBSD and DragonFly ship the BSD `pw` tool; NetBSD and
# OpenBSD share the user(8) suite (useradd/usermod/groupadd), differing only in
# the password-hash tool (pwhash vs encrypt). illumos/Solaris (SunOS) use the
# SVR4 useradd/groupadd suite, with the password written to /etc/shadow.
#
# The default arm hard-fails on any unrecognised kernel: every OS that reaches
# this script must have an explicit provisioning toolchain, otherwise it would
# silently fall through to the Linux/glibc branches and mis-provision users.
# When adding a new VM platform, add its kernel here with the correct USER_TOOL.
USER_TOOL=""
case "$OS_KERNEL" in
    Linux) USER_TOOL="" ;;
    FreeBSD | DragonFly) USER_TOOL="pw" ;;
    NetBSD | OpenBSD) USER_TOOL="user" ;;
    SunOS) USER_TOOL="svr4" ;;
    *)
        echo "ERROR: unsupported OS kernel '$OS_KERNEL'; no user-provisioning toolchain defined" >&2
        exit 1
        ;;
esac

# Hash a plaintext password for `usermod -p` on the user(8) suite (NetBSD and
# OpenBSD only; other kernels use pw, chpasswd or the SVR4 shadow path and never
# call this). NetBSD ships pwhash; OpenBSD hashes via encrypt(1) on stdin.
hash_afp_password() {
    afp_pw=$1
    case "$OS_KERNEL" in
        NetBSD) pwhash "$afp_pw" ;;
        OpenBSD) echo "$afp_pw" | encrypt ;;
        *)
            echo "ERROR: no password-hash tool for OS kernel '$OS_KERNEL'" >&2
            exit 1
            ;;
    esac
    return $?
}

# Set a user's password on the SVR4 suite (illumos/Solaris): there is no
# `usermod -p`, and passwd(1) is interactive, so write a crypt(3C) hash into
# /etc/shadow. openssl passwd -1 emits md5crypt ($1$), which illumos crypt(3C)
# verifies (BSD/Linux compatibility), so PAM's pam_unix_auth accepts it.
set_svr4_password() {
    svr4_user=$1
    svr4_pw=$2
    svr4_hash=$(openssl passwd -1 "$svr4_pw")
    svr4_tmp=$(mktemp)
    awk -F: -v u="$svr4_user" -v h="$svr4_hash" \
        'BEGIN { OFS = ":" } $1 == u { $2 = h } { print }' \
        /etc/shadow > "$svr4_tmp" && cat "$svr4_tmp" > /etc/shadow
    svr4_rc=$?
    rm -f "$svr4_tmp"
    return $svr4_rc
}

# --------------------------------------------------------------------------
# Path configuration
# --------------------------------------------------------------------------
# These default to the container layout so behavior is unchanged there. The
# non-container CI VMs (which install under a different prefix) override them
# via the environment before sourcing this script.

NETATALK_CONFDIR="${NETATALK_CONFDIR:-/etc/netatalk}"
NETATALK_SHARE_DIR="${NETATALK_SHARE_DIR:-/mnt/afpshare}"
NETATALK_BACKUP_DIR="${NETATALK_BACKUP_DIR:-/mnt/afpbackup}"
NETATALK_LOG_FILE="${NETATALK_LOG_FILE:-/var/log/afpd.log}"
if [ "$DISTRO" = "$DISTRO_ALPINE" ]; then
    NETATALK_LOCKDIR="${NETATALK_LOCKDIR:-/run/lock}"
else
    NETATALK_LOCKDIR="${NETATALK_LOCKDIR:-/var/lock}"
fi

# ensure the conf dir exists before we write afppasswd/afp.conf/extmap.conf
mkdir -p "$NETATALK_CONFDIR"

# --------------------------------------------------------------------------
# Validate required environment variables and create users / groups
# --------------------------------------------------------------------------

echo "*** Setting up first AFP user"

if [ -z "$AFP_USER" ]; then
    echo "ERROR: AFP_USER needs to be set to use this container." >&2
    exit 1
fi
if [ -z "$AFP_PASS" ]; then
    echo "ERROR: AFP_PASS needs to be set to use this container." >&2
    exit 1
fi
if [ -z "$AFP_GROUP" ]; then
    echo "ERROR: AFP_GROUP needs to be set to use this container." >&2
    exit 1
fi

if [ "$USER_TOOL" = "pw" ]; then
    if ! pw groupshow "$AFP_GROUP" > /dev/null 2>&1; then
        if [ -n "$AFP_GID" ]; then
            pw groupadd "$AFP_GROUP" -g "$AFP_GID"
        else
            pw groupadd "$AFP_GROUP"
        fi
    fi
    if ! pw usershow "$AFP_USER" > /dev/null 2>&1; then
        uidcmd=""
        [ -n "$AFP_UID" ] && uidcmd="-u $AFP_UID"
        pw useradd "$AFP_USER" $uidcmd -g "$AFP_GROUP" -d /nonexistent -s /usr/sbin/nologin -c "" -w no
    fi
    pw groupmod "$AFP_GROUP" -m "$AFP_USER"
elif [ "$USER_TOOL" = "user" ]; then
    if ! groupinfo -e "$AFP_GROUP"; then
        if [ -n "$AFP_GID" ]; then
            groupadd -g "$AFP_GID" "$AFP_GROUP"
        else
            groupadd "$AFP_GROUP"
        fi
    fi
    if ! userinfo -e "$AFP_USER"; then
        uidcmd=""
        [ -n "$AFP_UID" ] && uidcmd="-u $AFP_UID"
        useradd $uidcmd -g "$AFP_GROUP" -d /nonexistent -s /sbin/nologin "$AFP_USER"
    fi
    usermod -G "$AFP_GROUP" "$AFP_USER"
elif [ "$USER_TOOL" = "svr4" ]; then
    if ! getent group "$AFP_GROUP" > /dev/null 2>&1; then
        if [ -n "$AFP_GID" ]; then
            groupadd -g "$AFP_GID" "$AFP_GROUP"
        else
            groupadd "$AFP_GROUP"
        fi
    fi
    if ! getent passwd "$AFP_USER" > /dev/null 2>&1; then
        uidcmd=""
        [ -n "$AFP_UID" ] && uidcmd="-u $AFP_UID"
        useradd $uidcmd -g "$AFP_GROUP" -G "$AFP_GROUP" -d /nonexistent -s /bin/false "$AFP_USER"
    fi
    usermod -G "$AFP_GROUP" "$AFP_USER"
elif [ "$DISTRO" = "$DISTRO_ALPINE" ]; then
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
    if [ -n "$AFP_UID" ]; then
        cmd="$cmd --uid $AFP_UID"
    fi
    if [ -n "$AFP_GID" ]; then
        cmd="$cmd --gid $AFP_GID"
        groupadd --gid $AFP_GID $AFP_USER 2> /dev/null || true
    fi
    adduser $cmd --no-create-home --disabled-password --gecos '' "$AFP_USER" 2> /dev/null || true
    groupadd $AFP_GROUP 2> /dev/null || true
    usermod -aG $AFP_GROUP $AFP_USER 2> /dev/null || true
fi

if [ "$USER_TOOL" = "pw" ]; then
    echo "$AFP_PASS" | pw usermod "$AFP_USER" -h 0
elif [ "$USER_TOOL" = "user" ]; then
    usermod -p "$(hash_afp_password "$AFP_PASS")" "$AFP_USER"
elif [ "$USER_TOOL" = "svr4" ]; then
    set_svr4_password "$AFP_USER" "$AFP_PASS"
else
    echo "$AFP_USER:$AFP_PASS" | chpasswd > /dev/null 2>&1
fi

RANDNUM_PASSWD_FILE="$NETATALK_CONFDIR/afppasswd"

if [ -f "$RANDNUM_PASSWD_FILE" ]; then
    rm -f "$RANDNUM_PASSWD_FILE"
fi

if [ -f "$NETATALK_CONFDIR/afppasswd.srp" ]; then
    rm -f "$NETATALK_CONFDIR/afppasswd.srp"
fi

# Use AFP_UAMS verbatim if set, otherwise build from defaults
if [ -n "$AFP_UAMS" ]; then
    UAMS="$AFP_UAMS"
else
    UAMS="uams_dhx2.so"
fi

# Determine which UAM setups are needed
RANDNUM_WANTED=0
case "$UAMS" in
    *uams_randnum.so*) RANDNUM_WANTED=1 ;;
    *) ;;
esac
[ -z "$AFP_UAMS" ] && [ -n "$INSECURE_AUTH" ] && RANDNUM_WANTED=1

SRP_WANTED=0
case "$UAMS" in
    *uams_srp.so*) SRP_WANTED=1 ;;
    *) ;;
esac
[ -z "$AFP_UAMS" ] && SRP_WANTED=1

# Creating credentials for the RandNum UAM
RANDNUM_OK=0
if [ "$RANDNUM_WANTED" = "1" ]; then
    if afppasswd -c -f -r; then
        if afppasswd -a "$AFP_USER" -f -r -w "$AFP_PASS" > /dev/null; then
            RANDNUM_OK=1
        fi
    else
        echo "ERROR: Failed to initialize RandNum password file; disabling RandNum UAM" >&2
    fi
fi

# Creating credentials for the SRP UAM
SRP_OK=0
if [ "$SRP_WANTED" = "1" ]; then
    afppasswd -c

    if afppasswd -a "$AFP_USER" -f -w "$AFP_PASS" > /dev/null; then
        SRP_OK=1
    fi
fi

# Optional second user
if [ -n "$AFP_DROPBOX" ]; then
    if [ "$USER_TOOL" = "pw" ]; then
        pw groupmod "$AFP_GROUP" -m nobody
    elif [ "$USER_TOOL" = "user" ] || [ "$USER_TOOL" = "svr4" ]; then
        usermod -G "$AFP_GROUP" nobody
    elif [ "$DISTRO" = "$DISTRO_ALPINE" ]; then
        addgroup nobody "$AFP_GROUP"
    else
        usermod -aG $AFP_GROUP nobody 2> /dev/null || true
    fi
elif [ -n "$AFP_USER2" ]; then
    echo "*** Setting up second AFP user"

    if [ "$USER_TOOL" = "pw" ]; then
        if ! pw usershow "$AFP_USER2" > /dev/null 2>&1; then
            pw useradd "$AFP_USER2" -g "$AFP_GROUP" -d /nonexistent -s /usr/sbin/nologin -c "" -w no
        fi
        pw groupmod "$AFP_GROUP" -m "$AFP_USER2"
    elif [ "$USER_TOOL" = "user" ]; then
        if ! userinfo -e "$AFP_USER2"; then
            useradd -g "$AFP_GROUP" -d /nonexistent -s /sbin/nologin "$AFP_USER2"
        fi
        usermod -G "$AFP_GROUP" "$AFP_USER2"
    elif [ "$USER_TOOL" = "svr4" ]; then
        if ! getent passwd "$AFP_USER2" > /dev/null 2>&1; then
            useradd -g "$AFP_GROUP" -G "$AFP_GROUP" -d /nonexistent -s /bin/false "$AFP_USER2"
        fi
        usermod -G "$AFP_GROUP" "$AFP_USER2"
    elif [ "$DISTRO" = "$DISTRO_ALPINE" ]; then
        adduser --no-create-home --disabled-password "$AFP_USER2" 2> /dev/null || true
        addgroup "$AFP_USER2" "$AFP_GROUP"
    else
        adduser --no-create-home --disabled-password --gecos '' "$AFP_USER2" 2> /dev/null || true
        usermod -aG $AFP_GROUP $AFP_USER2 2> /dev/null || true
    fi

    if [ "$USER_TOOL" = "pw" ]; then
        echo "$AFP_PASS2" | pw usermod "$AFP_USER2" -h 0
    elif [ "$USER_TOOL" = "user" ]; then
        usermod -p "$(hash_afp_password "$AFP_PASS2")" "$AFP_USER2"
    elif [ "$USER_TOOL" = "svr4" ]; then
        set_svr4_password "$AFP_USER2" "$AFP_PASS2"
    else
        echo "$AFP_USER2:$AFP_PASS2" | chpasswd > /dev/null 2>&1
    fi

    if [ "$RANDNUM_WANTED" = "1" ] && ! afppasswd -a "$AFP_USER2" -f -r -w "$AFP_PASS2" > /dev/null; then
        RANDNUM_OK=0
    fi
    if [ "$SRP_WANTED" = "1" ] && ! afppasswd -a "$AFP_USER2" -f -w "$AFP_PASS2" > /dev/null; then
        SRP_OK=0
    fi
fi

if [ -z "$AFP_UAMS" ]; then
    if [ "$RANDNUM_OK" = "1" ]; then
        UAMS="$UAMS uams_randnum.so"
    elif [ -n "$INSECURE_AUTH" ]; then
        echo "NOTE: uams_randnum.so will not be loaded"
    fi
    if [ "$SRP_OK" = "1" ]; then
        UAMS="$UAMS uams_srp.so"
    else
        echo "NOTE: uams_srp.so will not be loaded"
    fi
else
    if [ "$RANDNUM_WANTED" = "1" ] && [ "$RANDNUM_OK" = "0" ]; then
        echo "WARNING: uams_randnum.so in AFP_UAMS but setup failed; it may not load" >&2
    fi
    if [ "$SRP_WANTED" = "1" ] && [ "$SRP_OK" = "0" ]; then
        echo "WARNING: uams_srp.so in AFP_UAMS but setup failed; it may not load" >&2
    fi
fi

# --------------------------------------------------------------------------
# Shared volumes and permissions
# --------------------------------------------------------------------------

echo "*** Configuring shared volume"
[ -d "$NETATALK_SHARE_DIR" ] || mkdir "$NETATALK_SHARE_DIR"
[ -d "$NETATALK_BACKUP_DIR" ] || mkdir "$NETATALK_BACKUP_DIR"

echo "*** Fixing permissions"
if [ -n "$AFP_DROPBOX" ]; then
    chmod 2755 "$NETATALK_SHARE_DIR"
    chmod 2775 "$NETATALK_BACKUP_DIR"
else
    chmod 2775 "$NETATALK_SHARE_DIR" "$NETATALK_BACKUP_DIR"
fi
if [ -n "$AFP_UID" ] && [ -n "$AFP_GID" ]; then
    chown "$AFP_UID:$AFP_GID" "$NETATALK_SHARE_DIR" "$NETATALK_BACKUP_DIR"
else
    chown "$AFP_USER:$AFP_GROUP" "$NETATALK_SHARE_DIR" "$NETATALK_BACKUP_DIR"
fi

if [ -n "$AFP_DROPBOX" ] && [ -z "$SHARE_NAME2" ]; then
    SHARE_NAME2="Dropbox"
fi

# --------------------------------------------------------------------------
# Lock file cleanup
# --------------------------------------------------------------------------

echo "*** Removing residual lock files"

# Ensure the lock directory exists before netatalk tries to create its lock
# file there (it exits silently, before logging is set up, if it cannot).
# The container images already ship this dir, so mkdir -p is a harmless no-op
# there; the BSD VMs may not have the compiled lock path (e.g. DragonflyBSD's
# /var/spool/locks), so this creates it.
mkdir -p "$NETATALK_LOCKDIR"
rm -f "$NETATALK_LOCKDIR/netatalk" "$NETATALK_LOCKDIR/atalkd" "$NETATALK_LOCKDIR/papd"

# --------------------------------------------------------------------------
# Netatalk configuration variables
# --------------------------------------------------------------------------

echo "*** Configuring Netatalk"
ATALK_NAME="${SERVER_NAME:-$(hostname | cut -d. -f1)}"

# Prepend so a -c (CSV, forces quiet) later in TEST_FLAGS wins over verbosity.
# VERBOSE enables normal test output (-v); VERY_VERBOSE also enables the
# testsuite's detailed diagnostic logging (-V).
[ -n "$VERBOSE" ] && TEST_FLAGS="-v $TEST_FLAGS"
[ -n "$VERY_VERBOSE" ] && TEST_FLAGS="-V $TEST_FLAGS"

if [ -z "$AFP_HOST" ]; then
    AFP_HOST="127.0.0.1"
fi

if [ -z "$AFP_PORT" ]; then
    AFP_PORT="548"
fi

if [ -z "$AFP_UAMS" ] && { [ -n "$INSECURE_AUTH" ] || [ -n "$AFP_DROPBOX" ]; }; then
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

AFP_SPOTLIGHT_GLOBAL="${AFP_SPOTLIGHT:-yes}"

if [ -n "$AFP_DROPBOX" ]; then
    AFP_VALIDUSERS1="$AFP_USER"
    AFP_VALIDUSERS2="$AFP_USER nobody"
else
    AFP_VALIDUSERS1="$AFP_USER $AFP_USER2"
    AFP_VALIDUSERS2="$AFP_USER $AFP_USER2"
fi

# --------------------------------------------------------------------------
# MySQL / MariaDB CNID backend
# --------------------------------------------------------------------------

sql_escape_identifier() {
    sql_identifier=$1
    printf '%s' "$sql_identifier" | sed 's/`/``/g'
    return $?
}

sql_escape_literal() {
    sql_literal=$1
    printf '%s' "$sql_literal" | sed "s/'/''/g"
    return $?
}

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
    if [ -z "$AFP_CNID_SQL_PASS" ]; then
        echo "*** Error: AFP_CNID_SQL_PASS must be set when AFP_CNID_BACKEND=mysql" >&2
        exit 1
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
        AFP_CNID_SQL_DB_SQL=$(sql_escape_identifier "$AFP_CNID_SQL_DB")
        AFP_CNID_SQL_USER_SQL=$(sql_escape_literal "$AFP_CNID_SQL_USER")
        AFP_CNID_SQL_PASS_SQL=$(sql_escape_literal "$AFP_CNID_SQL_PASS")
        mariadb -u root << EOSQL
CREATE DATABASE IF NOT EXISTS \`$AFP_CNID_SQL_DB_SQL\`;
CREATE USER IF NOT EXISTS '$AFP_CNID_SQL_USER_SQL'@'localhost' IDENTIFIED BY '$AFP_CNID_SQL_PASS_SQL';
GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, ALTER ON \`$AFP_CNID_SQL_DB_SQL\`.* TO '$AFP_CNID_SQL_USER_SQL'@'localhost';
CREATE USER IF NOT EXISTS '$AFP_CNID_SQL_USER_SQL'@'127.0.0.1' IDENTIFIED BY '$AFP_CNID_SQL_PASS_SQL';
GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, ALTER ON \`$AFP_CNID_SQL_DB_SQL\`.* TO '$AFP_CNID_SQL_USER_SQL'@'127.0.0.1';
FLUSH PRIVILEGES;
EOSQL
    fi
fi

# --------------------------------------------------------------------------
# Testsuite and config file flags
# --------------------------------------------------------------------------

if [ "$TESTSUITE" = "spectest" ] && [ -z "$AFP_REMOTE" ]; then
    TEST_FLAGS="$TEST_FLAGS -c $NETATALK_SHARE_DIR"
fi

# When the server enables 'afp read locks', tell afp_spectest (-L) so the
# byte-range read-lock conflict tests run instead of skipping (T_LOCKING).
# -L is an afp_spectest-only flag, so gate it on the spectest suite to avoid
# passing an unknown option to the other test runners.
if [ "$TESTSUITE" = "spectest" ] && [ "$AFP_READ_LOCKS" = "yes" ]; then
    TEST_FLAGS="$TEST_FLAGS -L"
fi

if [ -n "$AFP_CONFIG_POLLING" ]; then
    echo "*** Starting config file polling"
    /config_watch.sh "$NETATALK_CONFDIR/afp.conf" "$AFP_CONFIG_POLLING" &
fi

# --------------------------------------------------------------------------
# Generate afp.conf (unless MANUAL_CONFIG is set)
# --------------------------------------------------------------------------

if [ -z "$MANUAL_CONFIG" ]; then
    cat << EOF > "$NETATALK_CONFDIR/afp.conf"
[Global]
appletalk = $AFP_DDP
afp listen = ${AFP_LISTEN:-0.0.0.0}
afp read locks = ${AFP_READ_LOCKS:-no}
cnid mysql host = $AFP_CNID_SQL_HOST
cnid mysql user = $AFP_CNID_SQL_USER
cnid mysql pw = $AFP_CNID_SQL_PASS
cnid mysql db = $AFP_CNID_SQL_DB
dircache size = ${AFP_DIRCACHESIZE:-65536}
dircache mode = ${AFP_DIRCACHE_MODE:-lru}
dircache validation freq = ${AFP_DIRCACHE_VALIDATION_FREQ:-1}
dircache rfork budget = ${AFP_DIRCACHE_RFORK_BUDGET:-0}
dircache rfork maxsize = ${AFP_DIRCACHE_RFORK_MAXSIZE:-1024}
legacy icon = $AFP_LEGACY_ICON
log file = $NETATALK_LOG_FILE
log level = default:${AFP_LOGLEVEL:-info}
login message = $AFP_LOGIN_MESSAGE
mimic model = $AFP_MIMIC_MODEL
server name = ${SERVER_NAME:-Netatalk File Server}
uam list = $UAMS
mac charset = ${AFP_MAC_CHARSET:-MAC_ROMAN}
unix charset = ${AFP_UNIX_CHARSET:-UTF8}
vol charset = ${AFP_VOL_CHARSET:-UTF8}
spotlight = $AFP_SPOTLIGHT_GLOBAL
spotlight backend = ${AFP_SPOTLIGHT_BACKEND:-cnid}
[${SHARE_NAME:-File Sharing}]
cnid scheme = ${AFP_CNID_BACKEND:-dbd}
ea = $AFP_EA
path = $NETATALK_SHARE_DIR
valid users = $AFP_VALIDUSERS1
volume name = ${SHARE_NAME:-File Sharing}
$AFP_RWRO = $AFP_VALIDUSERS1
convert appledouble = ${AFP_CONVERT_APPLEDOUBLE:-no}
spotlight = $AFP_SPOTLIGHT_GLOBAL
[${SHARE_NAME2:-Time Machine}]
cnid scheme = ${AFP_CNID_BACKEND:-dbd}
ea = $AFP_EA
path = $NETATALK_BACKUP_DIR
time machine = $TIMEMACHINE
valid users = $AFP_VALIDUSERS2
volume name = ${SHARE_NAME2:-Time Machine}
$AFP_RWRO = $AFP_VALIDUSERS2
convert appledouble = ${AFP_CONVERT_APPLEDOUBLE:-no}
spotlight = $AFP_SPOTLIGHT_GLOBAL
EOF
fi

if [ -n "$AFP_EXTMAP" ]; then
    # The -i flag is not portable: GNU/NetBSD sed take an optional attached
    # suffix, while FreeBSD/DragonFly/macOS sed require a separate suffix arg
    # ('' = no backup). Use a temp file + mv instead, which works everywhere.
    extmap_file="$NETATALK_CONFDIR/extmap.conf"
    sed 's/^#\./\./' "$extmap_file" > "$extmap_file.tmp" \
        && mv "$extmap_file.tmp" "$extmap_file"
fi

# --------------------------------------------------------------------------
# AppleTalk (DDP) services
# --------------------------------------------------------------------------

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

# --------------------------------------------------------------------------
# AFP protocol version default
# --------------------------------------------------------------------------

if [ -z "$AFP_VERSION" ]; then
    AFP_VERSION=7
fi

# --------------------------------------------------------------------------
# I/O monitoring proc mount (for lantest)
# --------------------------------------------------------------------------

if [ "$TESTSUITE" = "lan" ]; then
    if [ -n "$IO_MONITORING" ]; then
        if mkdir -p /proc_io && mount -t proc -o hidepid=0,gid=0 proc /proc_io 2> /dev/null; then
            echo "Successfully created /proc_io with hidepid=0,gid=0 for I/O monitoring (gid=0 as TESTSUITE tests run as root)"
        else
            echo "WARNING: IO_MONITORING enabled, however failed to create /proc_io mount" >&2
            echo "NOTE: Container must be run with '--privileged' argument to allow proc mounts for afp_lantest IO monitoring" >&2
        fi
    else
        echo "NOTE: TESTSUITE set to 'lan' for afp_lantest test, however IO_MONITORING=1 envvar not set (mounts /proc_io filesystem). IO monitoring disabled."
    fi
fi
