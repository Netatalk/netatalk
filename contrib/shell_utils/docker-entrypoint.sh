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

set -o errexit -o errtrace -o functrace

function helper::configure() {
	if [ -z "${AFP_USER}" ] || [ -z "${AFP_PASS}" ]; then
		echo "AFP_USER and AFP_PASS env variables must be defined."
		exit 1
	fi

	echo "*** Setting up users and groups"

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

	echo "*** Configuring shared volume"

	if [ ! -d /mnt/afpshare ]; then
	  mkdir /mnt/afpshare
	  echo "Warning: this shared volume will be lost when the container is stopped."
	  echo "Use a Docker volume to retain your data between runs."
	fi

	echo "*** Fixing permissions"

	# Workarounds for afpd being weird about the permissions of the shared volume root
	chmod 2775 /mnt/afpshare
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

	echo "*** Configuring AppleVolumes.default"

	# Clean up residual configurations
	sed -i '/^~/d' /usr/local/etc/netatalk/AppleVolumes.default
	sed -i '/^\//d' /usr/local/etc/netatalk/AppleVolumes.default

	# Modify netatalk configuration files
	if [ -z "${SHARE_NAME}" ]; then
	    echo "/mnt/afpshare ${AVOLUMES_OPTIONS}" | tee -a /usr/local/etc/netatalk/AppleVolumes.default
	else
	    echo "/mnt/afpshare \"${SHARE_NAME}\" ${AVOLUMES_OPTIONS}" | tee -a /usr/local/etc/netatalk/AppleVolumes.default
	fi

	echo "*** Configuring afpd.conf"

	AFPD_DEFAULT_OPTIONS="-transall -setuplog \"default log_${AFPD_LOGLEVEL:-info} /dev/stdout\""
	AFPD_UAMS="-uamlist uams_dhx.so,uams_dhx2.so,uams_randnum.so"

	if [ ! -z "${INSECURE_AUTH}" ]; then
		AFPD_UAMS+=",uams_clrtxt.so,uams_guest.so"
	fi
	if [ -z "${SERVER_NAME}" ]; then
		echo "- ${AFPD_DEFAULT_OPTIONS} ${AFPD_UAMS} ${AFPD_OPTIONS}" | tee /usr/local/etc/netatalk/afpd.conf
	else
		echo "\"${SERVER_NAME}\" ${AFPD_DEFAULT_OPTIONS} ${AFPD_UAMS} ${AFPD_OPTIONS}" | tee /usr/local/etc/netatalk/afpd.conf
	fi

	echo "*** Configuring atalkd.conf"

	echo "${ATALKD_INTERFACE} ${ATALKD_OPTIONS}" | tee /usr/local/etc/netatalk/atalkd.conf

	echo "*** Configuring papd.conf"

	echo "cupsautoadd:op=root:" | tee /usr/local/etc/netatalk/papd.conf
}


[ -z "${MANUAL_CONFIG}" ] && helper::configure

# Release locks incase they're left behind from a previous execution
echo "*** Removing old locks"
rm -f /var/lock/afpd /var/lock/atalkd /var/lock/cnid_metad /var/lock/papd || true

# Ready to start things below here

# All daemons fork when ready, so we can launch them in order
if [ -z "${ATALKD_INTERFACE}" ]; then
	echo "WARNING The AppleTalk services will NOT be started. The requirements are:"
	echo "- The host OS has an AppleTalk networking stack, e.g. Linux or NetBSD."
	echo "- The Docker container uses the \`host' network driver with the \`NET_ADMIN' capability."
	echo "- The \`ATALKD_INTERFACE' environment variable is set to a valid host network interface."
else
	echo "*** Starting AppleTalk services (this will take a minute)"
	atalkd
	papd
	timelord -l
	a2boot
fi

echo "*** Starting AFP server"

# Prevent afpd from forking with '-d' parameter, to maintain container lifecycle
cnid_metad
afpd -d
