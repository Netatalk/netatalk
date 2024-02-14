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
        cmd="$cmd --uid ${AFP_UID}"
    fi
    if [ ! -z "${AFP_GID}" ]; then
        cmd="$cmd --gid ${AFP_GID}"
        groupadd --gid ${AFP_GID} ${AFP_USER} || true 2> /dev/null
    fi
    adduser $cmd --no-create-home --disabled-password --gecos '' "${AFP_USER}" || true 2> /dev/null
    if [ ! -z "${AFP_GROUP}" ]; then
        groupadd ${AFP_GROUP} || true 2> /dev/null
        usermod -aG "${AFP_GROUP}" "${AFP_USER}" || true 2> /dev/null
    fi
    if [ ! -z "${AFP_PASS}" ]; then
        echo "${AFP_USER}:${AFP_PASS}" | chpasswd
    fi
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

if [ -z "${MANUAL_CONFIG}" ]; then
    echo "*** Configuring Netatalk"
    cat <<EOF > /usr/etc/afp.conf
[Global]
log file = /var/log/afpd.log
log level = default:${AFP_LOGLEVEL:-info}
spotlight = yes
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
