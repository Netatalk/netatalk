#!/usr/bin/perl
# Display a form for editing afpd's extension mapping file

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

&ui_print_header(undef, $text{'edit_title_extmap'}, "", "extmap", 1);

print &ui_form_start("save_extmap.cgi", "post");
print &ui_table_start(undef, undef, 2);
print &ui_table_row(undef,
	&ui_textarea("extmap", &read_file_contents($config{'extmap_c'}), 24, 80), 2);
print &ui_table_end();
print &ui_form_end([[undef, $text{'save_button_title'}]]);

&ui_print_footer("index.cgi?tab=general", $text{'edit_return'});
