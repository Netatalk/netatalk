#!/usr/bin/env python3

# Convert a hex string to a C array of bytes.
# This can be used to take the ICN# hex data from ResEdit
# and convert it to a C array of bytes for use in etc/afpd/icon.h
#
# (c) 2024 Daniel Markstedt <daniel@mindani.net>
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

import sys

def process_hex_string(hex_string):
    if not all(c in "0123456789ABCDEFabcdef" for c in hex_string):
        print("Invalid input. Please provide a valid hexadecimal string.")
        sys.exit(1)

    hex_pairs = [hex_string[i:i+2] for i in range(0, len(hex_string), 2)]

    formatted_pairs = [f"0x{pair.upper()}" for pair in hex_pairs]

    lines = [
        ",  ".join(formatted_pairs[i:i+8]) 
        for i in range(0, len(formatted_pairs), 8)
    ]

    return "    " + ",\n    ".join(lines)

def main():
    if len(sys.argv) < 2:
        print("Usage: ./icn_hex_to_c.py <hex_string>")
        sys.exit(1)

    hex_string = sys.argv[1]
    print(process_hex_string(hex_string))

if __name__ == "__main__":
    main()
