#!/usr/bin/perl
# Display a form for editing AppleTalk interface settings

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

my @atalk_ifs = getAtalkIfs();

&ui_print_header(undef, $text{'show_atalk_title'}, "", "atalk", 1);
print "<h3>$text{index_show_atalk_title}</h3>\n";
print "<p>$text{'show_atalk_notice'}</p>";
my @atalk_links = ( "<a href=\"edit_atalk.cgi?action=create\">$text{'index_create_atalk'}</a>" );

if (@atalk_ifs) {
	unshift @atalk_links, (
		&select_all_link('section_index', 1),
		&select_invert_link('section_index', 1)
	);
	print &ui_form_start('delete_atalk.cgi', 'POST');
	print &ui_columns_start([
			'',
			$text{'show_atalk_iface'},
			$text{'show_atalk_routing'},
			$text{'show_atalk_phase'},
			$text{'show_atalk_net'},
			$text{'show_atalk_addr'},
			$text{'show_atalk_zone'}
		], undef, 0, undef, undef);
        my $index = 0;
	foreach $if (@atalk_ifs) {
		print &ui_columns_row([
			&ui_checkbox('section_index', $if->{atalk_iface}),
			"<a href=\"edit_atalk.cgi?action=edit&index=".$index."\">"
			.$if->{atalk_iface}."</a>",
			$if->{atalk_routing} ? $if->{atalk_routing} : $text{'index_value_not_set'},
			$if->{atalk_phase} ? $if->{atalk_phase} : $text{'index_value_not_set'},
			$if->{atalk_net} ? $if->{atalk_net} : $text{'index_value_not_set'},
			$if->{atalk_addr} ? $if->{atalk_addr} : $text{'index_value_not_set'},
			$if->{atalk_zone} ? $if->{atalk_zone} : $text{'index_value_not_set'}
		], [ "width='20'" ]);
                $index++;
	}
	print &ui_columns_end();
	print &ui_links_row(\@atalk_links);
	print &ui_form_end([[undef, $text{'index_delete_atalk_ifs'}, 0, undef]]);
} else {
	print "<p><b>$text{'index_no_atalk_ifs'}</b></p>\n";
	print &ui_links_row(\@atalk_links);
}

&ui_print_footer("index.cgi", $text{'edit_return'});
