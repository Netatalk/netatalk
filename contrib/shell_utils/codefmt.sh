#!/bin/bash

# Recursively format source files in the current repo directory
#
# (c) 2025 Daniel Markstedt <daniel@mindani.net>
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

if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    echo "Error: Not inside a git repository"
    exit 3
fi

FORMATTER_CMD=""
SOURCE_TYPE=""
VERBOSE=0

usage() {
    echo "Usage: $0 [-v] [-s C|meson]"
    exit 2
}

while getopts "vs:" opt; do
  case $opt in
    v)
      VERBOSE=1
      ;;
    s)
      SOURCE_TYPE="$OPTARG"
      if [ "$SOURCE_TYPE" != "C" ] && [ "$SOURCE_TYPE" != "meson" ]; then
        echo "Error: Source type must be either 'C' or 'meson'"
        usage
        exit 2
      fi
      ;;
    *)
      usage
      exit 2
      ;;
  esac
done

if [ -z "$SOURCE_TYPE" ]; then
    echo "Error: Source type (-s) is required"
    usage
fi

git diff --quiet HEAD
initial_state=$?

if [ "$SOURCE_TYPE" = "meson" ]; then
    if command -v muon >/dev/null 2>&1; then
        FORMATTER_CMD="muon"
    elif command -v muon-meson >/dev/null 2>&1; then
        FORMATTER_CMD="muon-meson"
    else
        echo "Error: No variant of muon found in PATH"
        exit 2
    fi
    if [ $VERBOSE -eq 1 ]; then
        find . -type f -name "meson.build" -exec sh -c 'echo "Processing file in: $(dirname {})" && '$FORMATTER_CMD' fmt -i {}' \;
    fi
    find . -type f -name "meson.build" -exec $FORMATTER_CMD fmt -i {} \;
elif [ "$SOURCE_TYPE" = "C" ]; then
    if command -v astyle >/dev/null 2>&1; then
        FORMATTER_CMD="astyle --options=.astylerc --recursive --suffix=none"
        if [ $VERBOSE -eq 0 ]; then
            FORMATTER_CMD="$FORMATTER_CMD --quiet"
        fi
    else
        echo "Error: astyle not found in PATH"
        exit 2
    fi
    $FORMATTER_CMD '*.h' '*.c'
fi

git diff --quiet HEAD
final_state=$?

if [ $initial_state -eq 0 ] && [ $final_state -eq 1 ]; then
    if [ $VERBOSE -eq 1 ]; then
        git --no-pager diff
        echo
    fi
    echo "reformatted $SOURCE_TYPE files to adhere to coding style guide"
    exit 1
elif [ $initial_state -eq 1 ] && [ $final_state -eq 1 ]; then
    echo "repo was dirty, please stash changes and try again"
    exit 2
else
    if [ $VERBOSE -eq 1 ]; then
        echo "beautiful, $SOURCE_TYPE files have compliant coding style!"
    fi
    exit 0
fi
