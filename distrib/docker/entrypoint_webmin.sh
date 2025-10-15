#!/bin/sh

# Entry point script for the netatalk webmin module container.
# Copyright (C) 2025  Daniel Markstedt <daniel@mindani.net>
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

set -e

[ -n "$DEBUG_ENTRY_SCRIPT" ] && set -x

handle_sigterm() {
    echo "*** Received SIGTERM, shutting down Webmin..."
    if [ -n "$MINISERV_PID" ] && kill -0 "$MINISERV_PID" 2> /dev/null; then
        kill "$MINISERV_PID"
        wait "$MINISERV_PID"
    fi
    exit 0
}

trap handle_sigterm TERM INT

echo "*** Setting up user"

if [ -z "$WEBMIN_USER" ]; then
    echo "ERROR: WEBMIN_USER needs to be set to use this container."
    exit 1
fi
if [ -z "$WEBMIN_PASS" ]; then
    echo "ERROR: WEBMIN_PASS needs to be set to use this container."
    exit 1
fi

adduser --disabled-password --gecos '' "$WEBMIN_USER" 2> /dev/null || true
usermod -aG sudo "$WEBMIN_USER"
echo "$WEBMIN_USER:$WEBMIN_PASS" | chpasswd

echo "*** Starting Webmin"

/usr/share/webmin/miniserv.pl --nofork /etc/webmin/miniserv.conf &
MINISERV_PID=$!

wait "$MINISERV_PID"
