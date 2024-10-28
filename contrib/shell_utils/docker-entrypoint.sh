#!/bin/bash

# Entry point script for netatalk docker container.
# Copyright (C) 2023  Eric Harmon
# Copyright (C) 2024  Daniel Markstedt
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

set -eo pipefail

echo "*** Setting up users and groups"

if [ -z "${AFP_USER}" ]; then
    echo "ERROR: AFP_USER needs to be set to use this Docker container."
    exit 1
fi
if [ -z "${AFP_PASS}" ]; then
    echo "ERROR: AFP_PASS needs to be set to use this Docker container."
    exit 1
fi
if [ -z "${AFP_GROUP}" ]; then
    echo "ERROR: AFP_GROUP needs to be set to use this Docker container."
    exit 1
fi

if [ ! -z "${AFP_UID}" ]; then
    uidcmd="-u ${AFP_UID}"
fi
if [ ! -z "${AFP_GID}" ]; then
    gidcmd="-g ${AFP_GID}"
fi

adduser ${uidcmd} --no-create-home --disabled-password "${AFP_USER}" || true 2> /dev/null
addgroup ${gidcmd} ${AFP_GROUP} || true 2> /dev/null
addgroup ${AFP_USER} ${AFP_GROUP}

echo "${AFP_USER}:${AFP_PASS}" | chpasswd

# Creating credentials for the RandNum UAM
set +e
if [ -f "/usr/local/etc/netatalk/afppasswd" ]; then
	rm -f /usr/local/etc/netatalk/afppasswd
fi
afppasswd -c
afppasswd -a -f -w "${AFP_PASS}" "${AFP_USER}"
if [ $? -ne 0 ]; then
    echo "NOTE: Use a password of 8 chars or less to authenticate with Mac OS 8 or earlier clients"
fi
set -e

# Crude way to create a second AFP user
if [ ! -z "${AFP_USER2}" ]; then
    adduser --no-create-home --disabled-password "${AFP_USER2}" || true 2> /dev/null
    addgroup ${AFP_USER2} ${AFP_GROUP}

    echo "${AFP_USER2}:${AFP_PASS2}" | chpasswd

    # Creating credentials for the RandNum UAM
    set +e
    afppasswd -a -f -w "${AFP_PASS2}" "${AFP_USER2}"
    if [ $? -ne 0 ]; then
        echo "NOTE: Use a password of 8 chars or less to authenticate with Mac OS 8 or earlier clients"
    fi
    set -e
fi

echo "*** Configuring shared volume"

if [ ! -d /mnt/afpshare ]; then
  mkdir /mnt/afpshare
  echo "Warning: the file sharing volume will be lost when the container is stopped."
  echo "Use a Docker volume to retain your data between runs."
fi

if [ ! -d /mnt/afpbackup ]; then
  mkdir /mnt/afpbackup
  echo "Warning: the Time Machine volume will be lost when the container is stopped."
  echo "Use a Docker volume to retain your data between runs."
fi

echo "*** Fixing permissions"

# Workarounds for afpd being weird about the permissions of the shared volume root
chmod 2775 /mnt/afpshare
chmod 2775 /mnt/afpbackup

if [ ! -z "${AFP_UID}" ] && [ ! -z "${AFP_GID}" ]; then
    chown "${AFP_UID}:${AFP_GID}" /mnt/afpshare /mnt/afpbackup
else
    chown "${AFP_USER}:${AFP_GROUP}" /mnt/afpshare /mnt/afpbackup
fi

echo "*** Removing residual lock files"

if [ -f "/var/lock/netatalk" ]; then
    rm -f /var/lock/netatalk
fi

if [ -f "/var/lock/atalkd" ]; then
    rm -f /var/lock/atalkd
fi

if [ -f "/var/lock/papd" ]; then
    rm -f /var/lock/papd
fi

UAMS="uams_dhx.so uams_dhx2.so uams_randnum.so"

if [ ! -z "${INSECURE_AUTH}" ]; then
    UAMS+=" uams_clrtxt.so uams_guest.so"
fi

if [ ! -z "${DISABLE_TIMEMACHINE}" ]; then
    TIMEMACHINE="no"
else
    TIMEMACHINE="yes"
fi

if [ -z "${SERVER_NAME}" ]; then
    ATALK_NAME=$(hostname|cut -d. -f1)
else
    ATALK_NAME=${SERVER_NAME}
fi

if [ ! -z "${AFP_READONLY}" ]; then
    AFP_RWRO="rolist"
else
    AFP_RWRO="rwlist"
fi

if [ ! -z "${VERBOSE}" ]; then
    TEST_FLAGS=-v
fi

if [ -z "${MANUAL_CONFIG}" ]; then
    echo "*** Configuring Netatalk"
    cat <<EOF > /usr/local/etc/afp.conf
[Global]
appletalk = yes
log file = /var/log/afpd.log
log level = default:${AFP_LOGLEVEL:-info}
spotlight = yes
uam list = ${UAMS}
zeroconf name = ${SERVER_NAME:-Netatalk File Server}
[${SHARE_NAME:-File Sharing}]
appledouble = ea
path = /mnt/afpshare
valid users = ${AFP_USER} ${AFP_USER2}
${AFP_RWRO} = ${AFP_USER} ${AFP_USER2}
[${SHARE2_NAME:-Time Machine}]
appledouble = ea
path = /mnt/afpbackup
time machine = ${TIMEMACHINE}
valid users = ${AFP_USER} ${AFP_USER2}
${AFP_RWRO} = ${AFP_USER} ${AFP_USER2}
EOF
fi

if [ -z "${ATALKD_INTERFACE}" ]; then
	echo "WARNING The AppleTalk services will NOT be started. The requirements are:"
	echo "- The host OS has an AppleTalk networking stack, e.g. Linux or NetBSD."
	echo "- The Docker container uses the \`host' network driver with the \`NET_ADMIN' capability."
	echo "- The \`ATALKD_INTERFACE' environment variable is set to a valid host network interface."
else
    echo "*** Configuring AppleTalk"
    echo "${ATALKD_INTERFACE} ${ATALKD_OPTIONS}" > /usr/local/etc/atalkd.conf
    echo "cupsautoadd:op=root:" > /usr/local/etc/papd.conf

	echo "*** Starting AppleTalk services (this will take a minute)"
 	cupsd
	atalkd
	nbprgstr -p 4 "${ATALK_NAME}:Workstation"
	nbprgstr -p 4 "${ATALK_NAME}:netatalk"
	papd
	timelord -l
	a2boot
fi

echo "*** Starting AFP server"

if [ -z "$TESTSUITE" ]; then
    # Prevent afpd from forking with '-d' parameter, to maintain container lifecycle
    netatalk -d
else
    netatalk
    sleep 2
    if [ "$TESTSUITE" == "tier1" ]; then
        afp_spectest "${TEST_FLAGS}" -"${AFP_VERSION}" -C -x -F "$TESTSUITE" -h 127.0.0.1 -p 548 -u "${AFP_USER}" -d "${AFP_USER2}" -w "${AFP_PASS}" -s "${SHARE_NAME}" -S "${SHARE2_NAME}"
    elif [ "$TESTSUITE" == "tier2" ]; then
        afp_spectest "${TEST_FLAGS}" -"${AFP_VERSION}" -C -x -F "$TESTSUITE" -h 127.0.0.1 -p 548 -u "${AFP_USER}" -d "${AFP_USER2}" -w "${AFP_PASS}" -s "${SHARE_NAME}" -S "${SHARE2_NAME}" -c /mnt/afpshare
    elif [ "$TESTSUITE" == "readonly" ]; then
        echo "testfile uno" > /mnt/afpshare/first.txt
        echo "testfile dos" > /mnt/afpshare/second.txt
        mkdir /mnt/afpshare/third
        afp_spectest "${TEST_FLAGS}" -"${AFP_VERSION}" -C -F "$TESTSUITE" -h 127.0.0.1 -p 548 -u "${AFP_USER}" -w "${AFP_PASS}" -s "${SHARE_NAME}"
    elif [ "$TESTSUITE" == "login" ]; then
        afp_logintest "${TEST_FLAGS}" -"${AFP_VERSION}" -C -h 127.0.0.1 -p 548 -u "${AFP_USER}" -w "${AFP_PASS}"
    elif [ "$TESTSUITE" == "encoding" ]; then
        afp_encodingtest "${TEST_FLAGS}" -"${AFP_VERSION}" -C -h 127.0.0.1 -p 548 -u "${AFP_USER}" -w "${AFP_PASS}"  -s "${SHARE_NAME}" -c /mnt/afpshare
    else
        echo "Unknown testsuite: ${TESTSUITE}"
        exit 1
    fi
fi
