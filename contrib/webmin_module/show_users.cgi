#!/usr/bin/perl
#
# Netatalk Webmin Module
# Copyright (C) 2013 Ralph Boehme <sloowfranklin@gmail.com>
# Copyright (C) 2023-4 Daniel Markstedt <daniel@mindani.net>
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

require 'netatalk-lib.pl';

ui_print_header(undef, $text{users_title}, "", "configs", 1, 1);

&ReadParse();

if ($in{kill}) {
	if (kill('TERM', $in{kill})) {
		print "<h4>$text{'users_disconnect_success'}</h4>\n";
	} else {
		print "<h4>$text{'users_disconnect_fail'}</h4>\n";
	}
}

my @users;
for (qx(ps aux)) {
	chomp;
	my @columns = split /\s+/;

	# Pick only lines that are afpd processes not owned by root
	if ($columns[10] =~ m/afpd/ && $columns[0] ne "root") {
		# Columns with index: 0=username, 1=PID, 8=date
		push @users, join(":::", $columns[0], $columns[1], $columns[8]);
	}
}

print "<p>",&text('users_connected_users', scalar(@users)),"</p>\n";
print &ui_columns_start([
		$text{'users_table_user'},
		$text{'users_table_connected'},
		$text{'users_table_pid'},
		$text{'users_table_action'}
	], undef, 0, undef, undef);
foreach my $user (sort @users) {
	my @line = split(":::", $user);
		print &ui_columns_row([
			$line[0],
			$line[2],
			$line[1],
			&ui_form_start('show_users.cgi', 'POST')
			.&ui_hidden('kill', $line[1])
			.&ui_form_end([[undef, $text{'users_button_disconnect'}]])
	]);
}
print &ui_columns_end();

ui_print_footer("index.cgi", $text{'edit_return'});
