#!/usr/bin/perl
# Control the netatalk processes

#    Netatalk Webmin Module
#    Copyright (C) 2000 Sven Mosimann/EcoLogic <sven.mosimann@ecologic.ch>
#    Copyright (C) 2000 Matthew Keller <kellermg@potsdam.edu>
#    Copyright (C) 2011 Steffan Cline <steffan@hldns.com>
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

my $daemon  = $in{'daemon'};
my $action  = $in{'action'};
my $command = $config{$action . '_' . $daemon};
my $rv      = system($command. ' </dev/null');
if ($rv) { die &text('init_failed', $command); }

if ($daemon ne 'netatalk') {
    redirect("index.cgi?tab=ddp");
} else {
    redirect("index.cgi");
}
