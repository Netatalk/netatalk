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

if [ ! -z "${AFP_USER}" ]; then
    if [ ! -z "${AFP_UID}" ]; then
        uidcmd="-u ${AFP_UID}"
    fi
    if [ ! -z "${AFP_GID}" ]; then
        gidcmd="-g ${AFP_GID}"
    fi
    if [ ! -z "${AFP_GROUP}" ]; then
        groupcmd="-G ${AFP_GROUP}"
        addgroup ${gidcmd} ${AFP_GROUP} || true 2> /dev/null
    fi
    adduser ${uidcmd} ${groupcmd} --no-create-home --disabled-password "${AFP_USER}" || true 2> /dev/null

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
    chown "${AFP_UID}:${AFP_GID}" /mnt/afpshare
    chown "${AFP_UID}:${AFP_GID}" /mnt/afpbackup
elif [ ! -z "${AFP_USER}" ] && [ ! -z "${AFP_GROUP}" ]; then
    chown "${AFP_USER}:${AFP_GROUP}" /mnt/afpshare
    chown "${AFP_USER}:${AFP_GROUP}" /mnt/afpbackup
elif [ ! -z "${AFP_USER}" ]; then
    chown "${AFP_USER}:${AFP_USER}" /mnt/afpshare
    chown "${AFP_USER}:${AFP_USER}" /mnt/afpbackup
fi

UAMS="uams_dhx.so uams_dhx2.so uams_randnum.so"

if [ ! -z "${INSECURE_AUTH}" ]; then
    UAMS+=" uams_clrtxt.so uams_guest.so"
fi

if [ -z "${MANUAL_CONFIG}" ]; then
    echo "*** Configuring Netatalk"
    cat <<EOF > /usr/local/etc/afp.conf
[Global]
log file = /var/log/afpd.log
log level = default:${AFP_LOGLEVEL:-info}
spotlight = yes
uam list = ${UAMS}
zeroconf name = ${SERVER_NAME:-Netatalk File Server}
[${SHARE_NAME:-File Sharing}]
path = /mnt/afpshare
valid users = ${AFP_USER}
[Time Machine]
path = /mnt/afpbackup
time machine = yes
valid users = ${AFP_USER}
EOF
fi

echo "*** Starting AFP server"

# Prevent afpd from forking with '-d' parameter, to maintain container lifecycle
netatalk -d
