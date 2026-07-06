#!/bin/sh

# Filesystem Change Event reporting helper for the 'fce notify script'
# afp.conf option.
#
# Copyright (C) 2014 Ralph Boehme <sloowfranklin@gmail.com>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.

usage="$(basename "$0") [-h] [-v version] [-e event] [-P path] [-S source path] -- FCE notification helper

where:
    -h  show this help text
    -v  version
    -e  event
    -P  path
    -S  source path for events like rename/move
    -u  username
    -p  pid
    -i  event ID

environment:
    NETATALK_FCE_LOG_FILE    append events to this file instead of syslog
    NETATALK_FCE_LOGGER      logger command path, default: logger
    NETATALK_FCE_LOG_TAG     syslog tag, default: netatalk-fce
"

while getopts ':hv:e:P:S:u:p:i:' option; do
    case "$option" in
        h)
            echo "$usage"
            exit
            ;;
        v)
            version=$OPTARG
            ;;
        e)
            event=$OPTARG
            ;;
        P)
            path=$OPTARG
            ;;
        S)
            srcpath=$OPTARG
            ;;
        u)
            user=$OPTARG
            ;;
        p)
            pid=$OPTARG
            ;;
        i)
            evid=$OPTARG
            ;;
        ?)
            printf "illegal option: '%s'\n" "$OPTARG" >&2
            echo "$usage" >&2
            exit 1
            ;;
    esac
done
shift $((OPTIND - 1))

message="FCE Event: ${event:-unknown}"

if [ -n "$version" ]; then
    message="$message, protocol: $version"
fi
if [ -n "$evid" ]; then
    message="$message, ID: $evid"
fi
if [ -n "$pid" ]; then
    message="$message, pid: $pid"
fi
if [ -n "$user" ]; then
    message="$message, user: $user"
fi
if [ -n "$srcpath" ]; then
    message="$message, source: $srcpath"
fi
if [ -n "$path" ]; then
    message="$message, path: $path"
fi

if [ -n "$NETATALK_FCE_LOG_FILE" ]; then
    printf '%s\n' "$message" >> "$NETATALK_FCE_LOG_FILE"
    exit $?
fi

logger_cmd="${NETATALK_FCE_LOGGER:-logger}"
logger_tag="${NETATALK_FCE_LOG_TAG:-netatalk-fce}"

if command -v "$logger_cmd" > /dev/null 2>&1; then
    "$logger_cmd" -t "$logger_tag" "$message"
    exit $?
fi

printf '%s\n' "$message" >&2
exit 0
