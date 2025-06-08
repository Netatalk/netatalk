#!/usr/bin/env perl

# Convert a hex string to a C array of bytes.
# This can be used to take the ICN# hex data from ResEdit
# and convert it to a C array of bytes for use in etc/afpd/icon.h
#
# (c) 2024-2025 Daniel Markstedt <daniel@mindani.net>
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

use strict;
use warnings;

sub process_hex_string {
    my ($hex_string) = @_;

    if ($hex_string !~ /^[0-9A-Fa-f]+$/) {
        print "Invalid input. Please provide a valid hexadecimal string.\n";
        exit 1;
    }

    my @hex_pairs       = $hex_string =~ /(..)/g;
    my @formatted_pairs = map { '0x' . uc($_) } @hex_pairs;
    my @lines;

    for (my $i = 0 ; $i < @formatted_pairs ; $i += 8) {
        push @lines, join(",  ", @formatted_pairs[$i .. ($i + 7 < $#formatted_pairs ? $i + 7 : $#formatted_pairs)]);
    }

    return "    " . join(",\n    ", @lines);
}

sub main {
    if (@ARGV < 1) {
        print "Usage: ./icn_hex_to_c.pl <hex_string>\n";
        exit 1;
    }

    my $hex_string = $ARGV[0];
    print process_hex_string($hex_string), "\n";
}

main();
