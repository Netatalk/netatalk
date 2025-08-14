#!/bin/sh

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

FORMATTER_CMD=""
SOURCE_TYPE=""
IS_GIT=0
VERBOSE=0

usage() {
    echo "Usage: $0 [-v] [-s c|markdown|meson|perl|shell|yaml]"
    echo "Run without arguments to format all source files recursively in working directory."
    exit 2
}

while getopts "vs:" opt; do
    case $opt in
        v)
            VERBOSE=1
            ;;
        s)
            SOURCE_TYPE="$OPTARG"
            if [ "$SOURCE_TYPE" != "c" ] && [ "$SOURCE_TYPE" != "markdown" ] && [ "$SOURCE_TYPE" != "meson" ] && [ "$SOURCE_TYPE" != "perl" ] && [ "$SOURCE_TYPE" != "shell" ] && [ "$SOURCE_TYPE" != "yaml" ]; then
                echo "Error: Source type must be either 'c', 'markdown', 'meson', 'perl', 'shell', or 'yaml'"
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

if ! git rev-parse --is-inside-work-tree > /dev/null 2>&1; then
    echo "Warning: Not inside a git repository; no diff will be produced"
else
    IS_GIT=1
    git diff --quiet HEAD
    INITIAL_STATE=$?
fi

if [ "$SOURCE_TYPE" = "c" ] || [ "$SOURCE_TYPE" = "" ]; then
    if command -v astyle > /dev/null 2>&1; then
        FORMATTER_CMD="astyle --options=.astylerc --recursive --suffix=none"
        if [ $VERBOSE -eq 1 ]; then
            echo "Formatting C sources..."
        else
            FORMATTER_CMD="$FORMATTER_CMD --quiet"
        fi
        eval "$FORMATTER_CMD '*.h' '*.c'"
    else
        echo "Error: astyle not found in PATH"
        exit 2
    fi
fi

if [ "$SOURCE_TYPE" = "markdown" ] || [ "$SOURCE_TYPE" = "" ]; then
    if command -v markdownlint-cli2 > /dev/null 2>&1; then
        markdownlint-cli2 --fix .
    else
        echo "Error: markdownlint-cli2 not found in PATH"
        exit 2
    fi
fi

if [ "$SOURCE_TYPE" = "meson" ] || [ "$SOURCE_TYPE" = "" ]; then
    if command -v muon > /dev/null 2>&1; then
        FORMATTER_CMD="muon"
    elif command -v muon-meson > /dev/null 2>&1; then
        FORMATTER_CMD="muon-meson"
    else
        echo "Error: No variant of muon found in PATH"
        exit 2
    fi
    if [ $VERBOSE -eq 1 ]; then
        echo "Formatting meson sources..."
        find . -type f -name "meson.build" -exec sh -c 'echo "Processing: $(dirname {})/meson.build" && '$FORMATTER_CMD' fmt -i {}' \;
    else
        find . -type f -name "meson.build" -exec $FORMATTER_CMD fmt -i {} \;
    fi
fi

if [ "$SOURCE_TYPE" = "perl" ] || [ "$SOURCE_TYPE" = "" ]; then
    if command -v perltidy > /dev/null 2>&1; then
        if [ "$VERBOSE" -eq 1 ]; then
            echo "Formatting Perl sources..."
            find . -type f \( -name "*.pl" -o -name "*.cgi" \) -exec sh -c '
                for file in "$@"; do
                    echo "Processing: $file"
                    perltidy --backup-file-extension="/" "$file"
                done
            ' sh {} +
        else
            find . -type f \( -name "*.pl" -o -name "*.cgi" \) -exec perltidy --backup-file-extension='/' {} +
        fi
    else
        echo "Error: perltidy not found in PATH"
        exit 2
    fi
fi

if [ "$SOURCE_TYPE" = "shell" ] || [ "$SOURCE_TYPE" = "" ]; then
    if command -v shfmt > /dev/null 2>&1; then
        if [ $VERBOSE -eq 1 ]; then
            echo "Formatting shell scripts..."
            find . -name "*.sh" -exec sh -c '
                for file in "$@"; do
                    echo "Processing: $file"
                    shfmt -w "$file"
                done
            ' sh {} +
        fi
        shfmt --write .
    else
        echo "Error: shfmt not found in PATH"
        exit 2
    fi
fi

if [ "$SOURCE_TYPE" = "yaml" ] || [ "$SOURCE_TYPE" = "" ]; then
    if command -v yamlfmt > /dev/null 2>&1; then
        if [ $VERBOSE -eq 1 ]; then
            echo "Formatting yaml files..."
            yamlfmt --verbose .
        else
            yamlfmt .
        fi
    else
        echo "Error: yamlfmt not found in PATH"
        exit 2
    fi
fi

if [ $IS_GIT -eq 1 ]; then
    git diff --quiet HEAD
    FINAL_STATE=$?
    if [ $INITIAL_STATE -eq 0 ] && [ $FINAL_STATE -eq 1 ]; then
        if [ $VERBOSE -eq 1 ]; then
            git --no-pager diff
        fi
        echo
        echo "reformatted source files to adhere to coding style guide"
        exit 1
    elif [ $INITIAL_STATE -eq 1 ] && [ $FINAL_STATE -eq 1 ]; then
        echo
        echo "repo was dirty, please stash changes and try again"
        exit 2
    else
        if [ $VERBOSE -eq 1 ]; then
            echo "beautiful, source files have compliant coding style!"
        fi
        exit 0
    fi
fi
