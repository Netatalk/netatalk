#!/usr/bin/perl

# Copyright (C) 2026 Daniel Markstedt <daniel@mindani.net>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.

use strict;
use warnings;
use FindBin;
use lib $FindBin::Bin;
use NadTest;

my $nad = shift @ARGV;
my $helper = shift @ARGV;
my $fixture = NadTest->new($nad);
system($helper, $fixture->volume, $fixture->work . '/afp.conf', $nad);
exit(($? & 127) ? 1 : $? >> 8);
