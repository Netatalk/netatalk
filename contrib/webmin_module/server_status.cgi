#!/usr/bin/perl
#
# Netatalk Webmin Module
# Copyright (C) 2024 Daniel Markstedt <daniel@mindani.net>
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
#

use strict;
use warnings;
require 'netatalk-lib.pl';
our %text;

ui_print_header(undef, $text{server_status_title}, "", "configs", 1, 1);

my $server_status = `asip-status localhost` || $text{'server_status_error'};

print "<p>" . $text{server_status_message} . "</p>";
print "<textarea name=\"status\" rows=\"26\" style=\"width: 75%; font-family: monospace; resize: none;\" readonly>";
print $server_status;
print "</textarea>";

ui_print_footer("index.cgi", $text{'edit_return'});
