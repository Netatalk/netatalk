#!/usr/bin/perl
# Save papd.conf

#    Netatalk Webmin Module
#    Copyright (C) 2024 Daniel Markstedt <daniel@mindani.net>
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

@printers = split(/\r?\n/, $in{'papd'});
&open_lock_tempfile(PAPD, ">$config{'papd_c'}");
foreach $p (@printers) {
	&print_tempfile(PAPD, $p,"\n");
	}
&close_tempfile(PAPD);

&redirect("index.cgi?tab=global");
