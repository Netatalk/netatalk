#!/usr/bin/perl
#
# Netatalk Webmin Module
# Copyright (C) 2024-2025 Daniel Markstedt <daniel@mindani.net>
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
our (%config, %in, %text);
&ReadParse();

my $afpconf;
eval { $afpconf = read_afpconf(); };
if ($@) {
    print $@;
    exit;
}

my @volume_options = map {
    my $name = $_->{parameters}{'volume name'}{value} || $_->{name};
    my $path = $_->{parameters}{'path'}{value};
    [$path, "$name ($path)"]
} @{$$afpconf{volumeSections}};

ui_print_header(undef, $text{index_icon_text_rebuild}, "", "rebuild", 1, 1);

# If a command was submitted, run it and show results
if (($in{'rebuild'} || $in{'status'}) && $in{'volpath'}) {

    my $dbd_path = `which dbd`;
    chomp($dbd_path);
    $dbd_path = '/usr/bin/dbd' unless $dbd_path;
    my $cmd     = $in{'rebuild'} ? "$dbd_path -f" : "$dbd_path -s";
    my $volpath = $in{'volpath'};
    # Shell-escape the path
    $volpath =~ s/(['"\\])/\\$1/g;
    $cmd .= " -F " . $config{'afp_conf'} . " " . $volpath;

    my @output;
    if (open(my $fh, "$cmd 2>&1 |")) {
        @output = <$fh>;
        close($fh);
    } else {
        @output = (&text('rebuild_db_command_failed', $!));
    }

    if (!@output) {
        if ($in{'rebuild'}) {
            @output = ($text{'rebuild_db_command_rebuild_success'});
        } else {
            @output = ($text{'rebuild_db_command_scan_success'});
        }
    }
    print "<h3>Command: <tt>" . $cmd . "</tt></h3>\n";
    print "<pre>" . join('', @output) . "</pre>";
    print "<br>";
    ui_print_footer("index.cgi?tab=general", $text{'edit_return'});
    exit;
}

print "<p>" . $text{'rebuild_db_command_help'} . "</p>";
print &ui_form_start('rebuild_db.cgi', 'post');
print &ui_select(
                 'volpath',
                 undef,
                 [map { [$_->[0], $_->[1]] } @volume_options]
);

print &ui_submit($text{'rebuild_db_button_status'},  'status');
print &ui_submit($text{'rebuild_db_button_rebuild'}, 'rebuild');
print &ui_form_end();

ui_print_footer("index.cgi?tab=general", $text{'edit_return'});
