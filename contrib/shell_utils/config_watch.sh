#!/bin/sh

# Simple script for polling for netatalk config file changes
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

WATCH_FILE="$1"
SLEEP_INTERVAL="$2"

get_checksum() {
    cksum "$WATCH_FILE" | awk '{ print $1 }'
}

LAST_CHECKSUM=$(get_checksum)

while true; do
    sleep "$SLEEP_INTERVAL"
    CURRENT_CHECKSUM=$(get_checksum)

    if [ "$CURRENT_CHECKSUM" != "$LAST_CHECKSUM" ]; then
        echo "$(date): Detected change in $WATCH_FILE"
        pkill -HUP netatalk
        echo "$(date): Sent SIGHUP to netatalk"
        LAST_CHECKSUM="$CURRENT_CHECKSUM"
    fi
done
