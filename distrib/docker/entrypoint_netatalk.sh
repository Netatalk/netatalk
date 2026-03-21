#!/bin/sh

# Entry point script for the netatalk container.
# Copyright (C) 2023  Eric Harmon
# Copyright (C) 2024-2026  Daniel Markstedt <daniel@mindani.net>
# Copyright (C) 2025-2026  Andy Lemin (@andylemin)
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

# Exit immediately on error
set -e

[ -n "$DEBUG_ENTRY_SCRIPT" ] && set -x

# --------------------------------------------------------------------------
# Common environment setup (users, volumes, config, services)
# --------------------------------------------------------------------------

. /env_setup.sh

# --------------------------------------------------------------------------
# Start AFP server and optionally run tests
# --------------------------------------------------------------------------

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
            echo "Unknown testsuite: $TESTSUITE" >&2
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
