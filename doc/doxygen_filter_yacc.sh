#!/bin/sh

# Simple doxygen filter script for yacc/bison .y grammar files.
# Strips yacc-specific syntax so Doxygen can parse the C code sections:
#
#   %{ ... %}    C prologue block — %{ / %} markers stripped, contents kept
#   %directive   Single-line and brace-block yacc directives — skipped entirely
#   %% ... %%    Grammar rules section — skipped entirely
#   (after %%)   C epilogue — passed through unchanged
#
# Copyright (C) 2026  Daniel Markstedt <daniel@mindani.net>
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

STATE=NORMAL
brace_depth=0

while IFS= read -r line; do
    case "$STATE" in

        NORMAL)
            case "$line" in
                '%%')
                    # First %% — enter grammar rules section
                    STATE=GRAMMAR
                    ;;
                '%{')
                    # C prologue block starts — keep contents, drop markers
                    STATE=C_BLOCK
                    ;;
                '%'*)
                    # Yacc directive (e.g. %token, %union, %code, %define, %type).
                    # If it opens a brace block, skip until the block closes.
                    STATE=YACC_DIRECTIVE
                    brace_depth=0
                    opens=$(printf '%s' "$line" | tr -cd '{' | wc -c)
                    closes=$(printf '%s' "$line" | tr -cd '}' | wc -c)
                    brace_depth=$((brace_depth + opens - closes))
                    if [ "$brace_depth" -le 0 ]; then
                        # Single-line directive with no open brace block
                        STATE=NORMAL
                    fi
                    ;;
                *)
                    printf '%s\n' "$line"
                    ;;
            esac
            ;;

        C_BLOCK)
            # Inside %{ ... %} — output C code, stop at %}
            case "$line" in
                '%}') STATE=NORMAL ;;
                *) printf '%s\n' "$line" ;;
            esac
            ;;

        YACC_DIRECTIVE)
            # Inside a brace-block yacc directive — skip lines, track depth
            opens=$(printf '%s' "$line" | tr -cd '{' | wc -c)
            closes=$(printf '%s' "$line" | tr -cd '}' | wc -c)
            brace_depth=$((brace_depth + opens - closes))
            if [ "$brace_depth" -le 0 ]; then
                STATE=NORMAL
            fi
            ;;

        GRAMMAR)
            # Between the two %% markers — skip grammar rules
            case "$line" in
                '%%') STATE=EPILOGUE ;;
                *) ;;
            esac
            ;;

        EPILOGUE)
            # C epilogue after second %% — pass through unchanged
            printf '%s\n' "$line"
            ;;

        *)
            ;;

    esac
done < "${1}"
