#!/usr/bin/perl
#
# Netatalk Webmin Module
# Copyright (C) 2013 Ralph Boehme <sloowfranklin@gmail.com>
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
my $tab;

eval {
	&ReadParse();

	if (exists $in{'tab'}) {
		$tab = $in{'tab'};
	} elsif ($in{'action'} eq "new_volume_preset" || $in{'action'} eq "edit_volume_preset" || $in{'action'} eq "new_homes" || $in{'action'} eq "edit_homes") {
		$tab = "general";
	} else {
		$tab = "fileserver";
	}

	# rejoin parameters that have been split for the user interface
	$in{'p_valid users'} = join_users_and_groups(defined $in{'pu_valid_users'} ? $in{'pu_valid_users'} : '', defined $in{'pg_valid_users'} ? $in{'pg_valid_users'} : '');
	$in{'p_invalid users'} = join_users_and_groups(defined $in{'pu_invalid_users'} ? $in{'pu_invalid_users'} : '', defined $in{'pg_invalid_users'} ? $in{'pg_invalid_users'} : '');
	$in{'p_rolist'} = join_users_and_groups(defined $in{'pu_rolist'} ? $in{'pu_rolist'} : '', defined $in{'pg_rolist'} ? $in{'pg_rolist'} : '');
	$in{'p_rwlist'} = join_users_and_groups(defined $in{'pu_rwlist'} ? $in{'pu_rwlist'} : '', defined $in{'pg_rwlist'} ? $in{'pg_rwlist'} : '');

	my $afpconfRef = &read_afpconf();
	modify_afpconf_ref_and_write($afpconfRef, \%in);

	redirect("index.cgi?tab=".$tab);
};
if($@) {
	my $msg = $@;

	ui_print_header(undef, $text{'errmsg_title'}, "", "configs", 1, 1);

	print "<p>$msg<p>";

	ui_print_footer("index.cgi?tab=".$tab, $text{'edit_return'});
	exit;
}
