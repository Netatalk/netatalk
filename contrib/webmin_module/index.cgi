#!/usr/bin/perl
#
# Netatalk Webmin Module
# Copyright (C) 2013 Ralph Boehme <sloowfranklin@gmail.com>
# Copyright (C) 2023-2025 Daniel Markstedt <daniel@mindani.net>
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
use File::Basename;
our (%config, %in, %text, $module_name);

&ReadParse();

my @tabs = (
            ['fileserver', $text{'index_tab_fileserver'}],
            ['general',    $text{'index_tab_general'}],
            ['ddp',        $text{'index_tab_ddp'}]
);

my $defaulttab = 'fileserver';
if (defined($in{'tab'})) {
    $defaulttab = $in{'tab'};
}

ui_print_header(&text('index_version', version()), $text{'index_title'}, "", "configs", 1, 1);

# check if netatalk daemon's path is configured correctly, if not: print error then exit
if (!-x $config{'netatalk_d'}) {
    print &text('index_ever', "<tt>$config{'netatalk_d'}</tt>", "/config.cgi?$module_name");
    exit;
}

# try to read and interpret afp.conf - if this doesn't work, print error and footer then exit
my $afpconf;
eval { $afpconf = &read_afpconf(); };
if ($@) {
    print $@;
    exit;
}

# since we are using a different number of forms, depending on the status of the service,
# we are keeping a running index while outputting the forms
my $current_formindex = 0;

print &ui_tabs_start(\@tabs, 'mode', $defaulttab);

print &ui_tabs_start_tab('mode', 'fileserver');

# Volumes
print "<h3>$text{index_volumes}</h3>\n";
my @volume_links = ("<a href=\"edit_vol_section.cgi?action=new_volume\">$text{'index_create_volume_link_name'}</a>");
if (@{$$afpconf{volumeSections}}) {
    unshift @volume_links, (
                            &select_all_link('section_index', $current_formindex),
                            &select_invert_link('section_index', $current_formindex)
    ) if (@{$$afpconf{volumeSections}} > 1);
    print &ui_form_start('delete_sections.cgi?tab=fileserver', 'post', undef, "id='volumes'");
    print &ui_columns_start(
                            [
                             '',
                             $text{'index_col_title_vol_name'},
                             $text{'index_col_title_path'},
                             $text{'index_col_title_uses_preset'}
                            ],
                            undef,
                            0,
                            undef,
                            undef
    );
    foreach my $volumeSection (sort { lc($a->{name}) cmp lc($b->{name}) } @{$$afpconf{volumeSections}}) {
        print &ui_columns_row(
            [
                &ui_checkbox('section_index', $$volumeSection{'index'}),
"<a href=\"edit_vol_section.cgi?action=edit_volume&tab=fileserver&index=$$volumeSection{'index'}\"><b>$$volumeSection{parameters}{'volume name'}{value}</b></a>",
                $$volumeSection{parameters}{'path'}{value},
                $$volumeSection{parameters}{'vol preset'}{value}
            ],
            ["width='20'"]
        );
    }
    print &ui_columns_end();
    print &ui_links_row(\@volume_links);
    print &ui_form_end([[undef, $text{'index_delete_volumes_button_title'}, 0, undef]]);
    $current_formindex += 1;
} else {
    print "<b>$text{'index_no_volumes'}</b>\n";
    print "<p>\n";
    print &ui_links_row(\@volume_links);
}

if (!$config{hide_service_controls}) {
    print &ui_hr();
    print "<h3>$text{'index_filesharing_services'}</h3>\n";

    # Process control Buttons
    if (&find_byname($config{'netatalk_d'})) {
        print &ui_buttons_start();
        print &ui_buttons_row(
                              'control.cgi?action=restart&daemon=' . basename($config{netatalk_d}),
                              $text{'running_restart'},
                              &text('index_process_control_restart_daemon', basename($config{netatalk_d}))
        );
        print &ui_buttons_row(
                              'control.cgi?action=stop&daemon=' . basename($config{netatalk_d}),
                              $text{'running_stop'},
                              &text('index_process_control_stop_daemon', basename($config{netatalk_d}))
        );
        print &ui_buttons_end();
        $current_formindex += 2;
    } else {
        print &ui_buttons_start();
        print &ui_buttons_row(
                              'control.cgi?action=start&daemon=' . basename($config{netatalk_d}),
                              $text{'running_start'},
                              &text('index_process_control_start_daemon', basename($config{netatalk_d}))
        );
        print &ui_buttons_end();
        $current_formindex += 1;
    }

    my @links_f = (
                   "server_status.cgi",
                   "show_users.cgi"
    );
    my @titles_f = (
                    $text{'index_icon_text_capabilities'},
                    $text{'index_icon_text_users'}
    );
    my @icons_f = (
                   "images/inspect.png",
                   "images/users.png"
    );
    icons_table(\@links_f, \@titles_f, \@icons_f);
}

print &ui_tabs_end_tab('mode', 'fileserver');
print &ui_tabs_start_tab('mode', 'general');

# Volume presets
print "<h3>$text{index_volume_presets}</h3>\n";
my @volume_preset_links = (
"<a href=\"edit_vol_section.cgi?action=new_volume_preset&tab=general\">$text{'index_create_volume_preset_link_name'}</a>"
);
if (@{$$afpconf{volumePresetSections}}) {
    # for an explanation of the following links, see above
    unshift @volume_preset_links, (
                                   &select_all_link('section_index', $current_formindex),
                                   &select_invert_link('section_index', $current_formindex)
    ) if (@{$$afpconf{volumePresetSections}} > 1);
    print &ui_form_start('delete_sections.cgi?tab=general', 'post', undef, "id='volume_presets'");
    print &ui_columns_start(
                            [
                             '',
                             $text{'index_col_title_preset_name'},
                             $text{'index_col_title_used_by'}
                            ],
                            undef,
                            0,
                            undef,
                            undef
    );
    foreach my $volumeSection (sort { lc($a->{name}) cmp lc($b->{name}) } @{$$afpconf{volumePresetSections}}) {
        print &ui_columns_row(
            [
                &ui_checkbox('section_index', $$volumeSection{'index'}),
"<a href=\"edit_vol_section.cgi?action=edit_volume_preset&tab=general&index=$$volumeSection{'index'}\"><b>$$volumeSection{name}</b></a>",
                defined $$volumeSection{presetUsedBySectionNames}
                ? join("<br>", @{$$volumeSection{presetUsedBySectionNames}})
                : ""
            ],
            ["width='20'"]
        );
    }
    print &ui_columns_end();
    print &ui_links_row(\@volume_preset_links);
    print &ui_form_end([[undef, $text{'index_delete_volume_presets_button_title'}, 0, undef]]);
    $current_formindex += 1;
} else {
    print "<b>$text{'index_no_volume_presets'}</b>\n";
    print "<p>\n";
    print &ui_links_row(\@volume_preset_links);
}
print &ui_hr();

# Homes
print "<h3>$text{index_homes}</h3>\n";
if ($$afpconf{sectionsByName}{'Homes'}) {
    print &ui_form_start('delete_sections.cgi?tab=general', 'post', undef, "id='homes'");
    print &ui_columns_start(
                            [
                             $text{'index_col_title_basedir_regex'},
                             $text{'index_col_title_home_path'},
                             $text{'index_col_title_home_name'},
                            ]
    );
    my $volumeSection = $$afpconf{sectionsByName}{'Homes'};
    my @basedir_regex = get_parameter_of_section($afpconf, $volumeSection, 'basedir regex');
    my @path          = get_parameter_of_section($afpconf, $volumeSection, 'path');
    my @home_name     = get_parameter_of_section($afpconf, $volumeSection, 'home name');
    print &ui_columns_row(
                 [
                    "<input type='hidden' name='section_index' value='$$volumeSection{'index'}'>"
                  . "<a href=\"edit_vol_section.cgi?action=edit_homes&tab=general&index=$$volumeSection{'index'}\"><b>"
                  . ($basedir_regex[0] ne '' ? html_escape($basedir_regex[0]) : $text{'index_value_not_set'})
                  . "</b></a>",
                  $path[0] ne ''      ? html_escape($path[0])      : $text{'index_value_not_set'},
                  $home_name[0] ne '' ? html_escape($home_name[0]) : $text{'index_value_not_set'},
                 ]
    );
    print &ui_columns_end();
    print &ui_form_end([[undef, $text{'index_delete_homes_button_title'}, 0, undef]]);
} else {
    print "<b>$text{'index_no_homes'}</b>\n";
    print "<p>\n";
    print &ui_links_row(
           ["<a href=\"edit_vol_section.cgi?action=new_homes&tab=general\">$text{'index_create_homes_link_name'}</a>"]);
}

print "<h3>$text{index_global}</h3>\n";

my @links_g = (
               "edit_global_section.cgi",
               "edit_extmap.cgi",
               "rebuild_db.cgi"
);
my @titles_g = (
                $text{'index_icon_text_server'},
                $text{'index_icon_text_extmap'},
                $text{'index_icon_text_rebuild'}
);
my @icons_g = (
               "images/server.png",
               "images/digest.png",
               "images/options.png",
);
icons_table(\@links_g, \@titles_g, \@icons_g);

print &ui_tabs_end_tab('mode', 'general');

print &ui_tabs_start_tab('mode', 'ddp');

my @atalk_ifs = getAtalkIfs();

print "<h3>$text{index_show_atalk_title}</h3>\n";
print "<p>$text{'show_atalk_notice'}</p>";
my @atalk_links = ("<a href=\"edit_atalk.cgi?action=create\">$text{'index_create_atalk'}</a>");

if (@atalk_ifs) {
    unshift @atalk_links, (
                           &select_all_link('section_index', 1),
                           &select_invert_link('section_index', 1)
    );
    print &ui_form_start('delete_atalk.cgi', 'POST');
    print &ui_columns_start(
                            [
                             '',
                             $text{'show_atalk_iface'},
                             $text{'show_atalk_routing'},
                             $text{'show_atalk_phase'},
                             $text{'show_atalk_net'},
                             $text{'show_atalk_addr'},
                             $text{'show_atalk_zone'}
                            ],
                            undef,
                            0,
                            undef,
                            undef
    );
    my $index = 0;
    foreach my $if (sort { lc($a->{atalk_iface}) cmp lc($b->{atalk_iface}) } @atalk_ifs) {
        print &ui_columns_row(
                              [
                               &ui_checkbox('section_index', $if->{atalk_iface}),
                               "<a href=\"edit_atalk.cgi?action=edit&index="
                               . $index . "\">"
                               . $if->{atalk_iface} . "</a>",
                               $if->{atalk_routing} ? $if->{atalk_routing} : $text{'index_value_not_set'},
                               $if->{atalk_phase}   ? $if->{atalk_phase}   : $text{'index_value_not_set'},
                               $if->{atalk_net}     ? $if->{atalk_net}     : $text{'index_value_not_set'},
                               $if->{atalk_addr}    ? $if->{atalk_addr}    : $text{'index_value_not_set'},
                               $if->{atalk_zone}    ? $if->{atalk_zone}    : $text{'index_value_not_set'}
                              ],
                              ["width='20'"]
        );
        $index++;
    }
    print &ui_columns_end();
    print &ui_links_row(\@atalk_links);
    print &ui_form_end([[undef, $text{'index_delete_atalk_ifs'}, 0, undef]]);
} else {
    print "<p><b>$text{'index_no_atalk_ifs'}</b></p>\n";
    print &ui_links_row(\@atalk_links);
}

if (!$config{hide_service_controls}) {
    print &ui_hr();
    my @daemons = (
                   {basename($config{atalkd_d})   => $text{index_process_atalkd}},
                   {basename($config{papd_d})     => $text{index_process_papd}},
                   {basename($config{timelord_d}) => $text{index_process_timelord}},
                   {basename($config{a2boot_d})   => $text{index_process_a2boot}}
    );

    print "<h3>$text{'index_appletalk_services'}</h3>\n";
    print "<p>$text{'index_appletalk_services_notice'}</p>";

    foreach my $daemon (@daemons) {
        foreach my $d (keys %$daemon) {
            if (-x $config{$d . '_d'}) {
                if (&find_byname($config{$d . '_d'})) {
                    print "<h3>" . &text('index_running_service', $daemon->{$d}) . "</h3>\n";
                    print &ui_buttons_start();
                    print &ui_buttons_row(
                                          'control.cgi?action=restart&daemon=' . $d,
                                          &text('running_restart_daemon',               $daemon->{$d}),
                                          &text('index_process_control_restart_daemon', $d)
                    );
                    print &ui_buttons_row(
                                          'control.cgi?action=stop&daemon=' . $d,
                                          &text('running_stop_daemon',               $daemon->{$d}),
                                          &text('index_process_control_stop_daemon', $d)
                    );
                    print &ui_buttons_end();
                    $current_formindex += 2;
                } else {
                    print "<h3>" . &text('index_not_running', $daemon->{$d}) . "</h3>\n";
                    print &ui_buttons_start();
                    print &ui_buttons_row(
                                          'control.cgi?action=start&daemon=' . $d,
                                          &text('running_start_daemon',               $daemon->{$d}),
                                          &text('index_process_control_start_daemon', $d)
                    );
                    print &ui_buttons_end();
                    $current_formindex += 1;
                }
            } else {
                print "<p>" . &text('index_daemon_not_found', $d) . "</p>";
            }
        }
    }
}

my @links_d  = ("edit_print.cgi");
my @titles_d = ($text{'index_icon_text_print'});
my @icons_d  = ("images/printer.png");
icons_table(\@links_d, \@titles_d, \@icons_d);

print &ui_tabs_end_tab('mode', 'ddp');

print &ui_tabs_end();
