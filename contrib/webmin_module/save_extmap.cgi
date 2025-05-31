#!/usr/bin/perl
# Save extmap.conf

#    Netatalk Webmin Module
#    Copyright (C) 2024-2025 Daniel Markstedt <daniel@mindani.net>
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.

require 'netatalk-lib.pl';

&ReadParse();

@mappings = split(/\r?\n/, $in{'extmap'});
&open_lock_tempfile(EXTMAP, ">$config{'extmap_c'}");
foreach $p (@mappings) {
    &print_tempfile(EXTMAP, $p, "\n");
}
&close_tempfile(EXTMAP);

&redirect("index.cgi?tab=general");
