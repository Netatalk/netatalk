#!/bin/sh

# Simple doxygen filter script to convert mermaid fenced code blocks in markdown files
# into HTML blocks that can be rendered by mermaid.js
# Rewritten as a POSIX shell compatible script from original by Tapani Hyvamaki
#
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

MERMAID_FENCE_START='```mermaid'
MERMAID_FENCE_END='```'

inside_mermaid_block=0

while IFS= read -r line; do
    case "$line" in
        "$MERMAID_FENCE_START"*)
            suffix="${line#"$MERMAID_FENCE_START"}"
            printf '\n<pre class="mermaid">%s\n' "$suffix"
            inside_mermaid_block=1
            ;;
        *)
            if [ "$inside_mermaid_block" -eq 1 ]; then
                if [ "$line" = "$MERMAID_FENCE_END" ]; then
                    printf '</pre>\n\n'
                    inside_mermaid_block=0
                else
                    printf '%s\n' "$line"
                fi
            else
                printf '%s\n' "$line"
            fi
            ;;
    esac
done < "${1}"
