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

eval {
	&ReadParse();

	# properly join multiple values for uam list and canonicalize
	$in{'p_uam list'} =~ s/\x00/ /g;
	$in{'p_uam list'} =~ s/^[ ,]+//; $in{'p_uam list'} =~ s/[ ,]+$//; $in{'p_uam list'} =~ s/[ ,]+/ /g;

	# correct fields which had to be named without spaces catering for sloppy name handling
    # in the webmin API
	$in{'p_k5 keytab'} = $in{'p_k5_keytab'}; delete $in{'p_k5_keytab'};
	$in{'p_vol dbpath'} = $in{'p_vol_dbpath'}; delete $in{'p_vol_dbpath'};
	$in{'p_log file'} = $in{'p_log_file'}; delete $in{'p_log_file'};

	my $afpconfRef = &read_afpconf();
	modify_afpconf_ref_and_write($afpconfRef, \%in);

	redirect("index.cgi?tab=general");
};
if($@) {
	my $msg = $@;

	ui_print_header(undef, $text{'errmsg_title'}, "", "configs", 1, 1);

	print "<p>$msg<p>";

	ui_print_footer("index.cgi?tab=general", $text{'edit_return'});
	exit;
}
