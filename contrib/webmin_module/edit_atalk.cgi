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

my $page_title;
my @atalk;

if ($in{action} =~ /create/) {
	$page_title = $text{'edit_atalk_new_title'};
}
elsif ($in{action} =~ /edit/) {
        @atalk = getAtalkIfs();
	$page_title = $text{'edit_atalk_title'};
}
else {
	die("Unknown action type. Are you trying something naughty?");
}

&ui_print_header(undef, $page_title, "", "atalk", 1);

print &ui_form_start('save_atalk.cgi', 'POST');

print &ui_table_start($text{'edit_atalk_table_heading'}, 'width="100%"', 2);

print &ui_table_row($text{'edit_atalk_iface'},
	&ui_textbox('atalk_iface', @atalk ? $atalk[$in{index}]{atalk_iface} : '', undef, undef, undef, 'required')
	."<br /><a href=\"/net/list_ifcs.cgi\" target=\"_blank\">$text{'edit_atalk_iface_help'}</a>"
);
print &ui_table_row($text{'edit_atalk_addr'},
	&ui_textbox('atalk_addr', @atalk ? $atalk[$in{index}]{atalk_addr} : '')
	." ".$text{'edit_atalk_addr_help'}
);
print &ui_table_row($text{'edit_atalk_net'},
	&ui_textbox('atalk_net', @atalk ? $atalk[$in{index}]{atalk_net} : '')
	." ".$text{'edit_atalk_net_help'}
);
print &ui_table_row($text{'edit_atalk_routing'},
	&ui_select('atalk_routing', @atalk ? $atalk[$in{index}]{atalk_routing} : '', [
                        [''],
			['-route', $text{edit_atalk_routing_route}],
			['-seed', $text{edit_atalk_routing_seed}],
			['-dontroute', $text{edit_atalk_routing_dontroute}]
		])
);
print &ui_table_row($text{'edit_atalk_phase'},
	&ui_select('atalk_phase', @atalk ? $atalk[$in{index}]{atalk_phase} : '', [
                        [''],
			['1'],
			['2']
		])
);
print &ui_table_row($text{'edit_atalk_zone'},
	&ui_textbox('atalk_zone', @atalk ? $atalk[$in{index}]{atalk_zone} : '')
);
print &ui_table_end();

if ($in{action} =~ /edit/) {
	print &ui_hidden('old_iface', $atalk[$in{index}]{atalk_iface});
}
print &ui_form_end([[undef, $text{'save_button_title'}]]);

&ui_print_footer("show_atalk.cgi", $text{'index_atalk'});
