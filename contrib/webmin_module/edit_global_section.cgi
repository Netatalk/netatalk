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

# all inputs for netatalk configuration parameters follow the naming
# convention "p_"+parameter name to keep the save_global_section.cgi simple

use strict;
use warnings;
require 'netatalk-lib.pl';
our (%in, %text);
my ($afpconfRef, $sectionRef);

eval {
    &ReadParse();

    # read afp.conf and check parameters
    $afpconfRef = &read_afpconf();

    $sectionRef = $$afpconfRef{sectionsByName}{'Global'} || die $text{edit_global_section_error} . "\n";
};
if ($@) {
    # preparations failed with an error message in $@ - print error

    my $msg = $@;

    ui_print_header(undef, $text{'errmsg_title'}, "", "configs", 1, 1);

    print "<p>$msg<p>";

    ui_print_footer("index.cgi", $text{'edit_return'});

    exit;
}

my @tabs = (
            ['common',  $text{'edit_global_section_tab_common'}],
            ['auth',    $text{'edit_global_section_tab_auth'}],
            ['network', $text{'edit_global_section_tab_network'}],
            ['misc',    $text{'edit_global_section_tab_misc'}],
            ['cnid',    $text{'edit_global_section_tab_cnid'}],
            ['acl',     $text{'edit_global_section_tab_acl'}],
            ['fce',     $text{'edit_global_section_tab_fce'}]
);

ui_print_header(undef, $text{'edit_global_section_title'}, "", "configs", 1, 1);

print &ui_form_start('save_global_section.cgi', 'POST');

print &ui_hidden('index', $$sectionRef{'index'}) if ($sectionRef);
print &ui_hidden('name',  'Global');

print &ui_tabs_start(\@tabs, 'mode', 'common');
print &ui_tabs_start_tab('mode', 'common');

print &ui_table_start($text{'edit_global_section_title_table'}, 'width="100%"', 2);

my @values = get_parameter_of_section($afpconfRef, $sectionRef, 'server name', \%in);
print &ui_table_row(
                    $text{'edit_global_section_server_name'},
                    &ui_textbox('p_server name', $values[0], 30) . " "
                    . $text{'edit_global_section_server_name_default'}
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'login message', \%in);
print &ui_table_row(
                    $text{'edit_global_section_login_message'},
                    &ui_textbox('p_login message', $values[0], 60)
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'uam list', \%in);
my $nonstandardUAMs = @values[0];
$nonstandardUAMs =~ s/uams_dhx2?.so|uams_clrtxt.so|uams_guest.so|uams_gss.so|uams_randnum.so//g;
$nonstandardUAMs =~ s/^[ ,]+//;
$nonstandardUAMs =~ s/[ ,]+$//;
$nonstandardUAMs =~ s/[ ,]+/ /g;
@values[0] = "uams_dhx.so uams_dhx2.so" if !@values[0];
print &ui_table_row(
                    $text{'edit_global_section_uam_list'},
                    &ui_checkbox('p_uam list', 'uams_dhx2.so', 'DHX2 UAM', $values[0] =~ /uams_dhx2.so/ ? 1 : 0)
                    . &ui_checkbox('p_uam list', 'uams_dhx.so', 'DHX UAM', $values[0] =~ /uams_dhx.so/  ? 1 : 0)
                    . &ui_checkbox(
                                   'p_uam list', 'uams_clrtxt.so', 'Cleartext UAM',
                                   $values[0] =~ /uams_clrtxt.so/ ? 1 : 0
                    )
                    . &ui_checkbox(
                                   'p_uam list', 'uams_randnum.so', 'RandNum UAM',
                                   $values[0] =~ /uams_randnum.so/ ? 1 : 0
                    )
                    . &ui_checkbox('p_uam list', 'uams_guest.so', 'Guest UAM',    $values[0] =~ /uams_guest.so/ ? 1 : 0)
                    . &ui_checkbox('p_uam list', 'uams_gss.so',   'Kerberos UAM', $values[0] =~ /uams_gss.so/   ? 1 : 0)
                    . "<br>"
                    . $text{'edit_global_section_uam_list_other'} . " "
                    . &ui_textbox('p_uam list', $nonstandardUAMs, 40)
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'log file', \%in);
print &ui_table_row(
                    $text{'edit_global_section_log_file'},
                    &ui_filebox('p_log_file', $values[0]) . " " . $text{'edit_global_section_log_file_default'}
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'log level', \%in);
print &ui_table_row(
                    $text{'edit_global_section_log_level'},
                    &ui_textbox('p_log level', $values[0], 30) . " "
                    . ($values[2] ? html_escape($values[2]) . ": " . html_escape($values[1]) : '') . "\n"
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'zeroconf', \%in);
print &ui_table_row(
                    $text{'edit_global_section_zeroconf'},
                    &build_select(
                                  $afpconfRef, $sectionRef, \%in, 'zeroconf', $text{'edit_undefined'}, 'yes', 'yes',
                                  'no', 'no'
                    )
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'appletalk', \%in);
print &ui_table_row(
                    $text{'edit_global_section_appletalk'},
                    &build_select(
                                  $afpconfRef, $sectionRef, \%in, 'appletalk', $text{'edit_undefined'}, 'yes', 'yes',
                                  'no', 'no'
                    )
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'spotlight', \%in);
print &ui_table_row(
                    $text{'edit_global_section_spotlight'},
                    &build_select(
                                  $afpconfRef, $sectionRef, \%in, 'spotlight', $text{'edit_undefined'}, 'yes', 'yes',
                                  'no', 'no'
                    )
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'afpstats', \%in);
print &ui_table_row(
                    $text{'edit_global_section_afpstats'},
                    &build_select(
                                  $afpconfRef, $sectionRef, \%in, 'afpstats', $text{'edit_undefined'}, 'yes', 'yes',
                                  'no', 'no'
                    )
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'mimic model', \%in);
print &ui_table_row(
                    $text{'edit_global_section_mimic_model'},
                    &ui_textbox('p_mimic model', $values[0], 20) . $text{edit_global_section_mimic_model_help}
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'legacy icon', \%in);
print &ui_table_row(
                    $text{'edit_global_section_legacy_icon'},
                    &ui_textbox('p_legacy icon', $values[0], 20) . $text{edit_global_section_legacy_icon_help}
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'mac charset', \%in);
print &ui_table_row(
                    $text{'edit_global_section_mac_charset'},
                    &build_select(
                                  $afpconfRef,
                                  $sectionRef,
                                  \%in,
                                  'mac charset',
                                  $text{'edit_undefined'},
                                  'MAC_CENTRALEUROPE',
                                  'MAC_CENTRALEUROPE',
                                  'MAC_CHINESE_SIMP',
                                  'MAC_CHINESE_SIMP',
                                  'MAC_CHINESE_TRAD',
                                  'MAC_CHINESE_TRAD',
                                  'MAC_CYRILLIC',
                                  'MAC_CYRILLIC',
                                  'MAC_GREEK',
                                  'MAC_GREEK',
                                  'MAC_HEBREW',
                                  'MAC_HEBREW',
                                  'MAC_JAPANESE',
                                  'MAC_JAPANESE',
                                  'MAC_KOREAN',
                                  'MAC_KOREAN',
                                  'MAC_ROMAN',
                                  'MAC_ROMAN',
                                  'MAC_TURKISH',
                                  'MAC_TURKISH'
                    )
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'unix charset', \%in);
print &ui_table_row(
                    $text{'edit_global_section_unix_charset'},
                    &ui_textbox('p_unix charset', $values[0], 10) . " "
                    . ($values[2] ? html_escape($values[2]) . ": " . html_escape($values[1]) : '') . "\n"
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'vol charset', \%in);
print &ui_table_row(
                    $text{'edit_global_section_vol_charset'},
                    &ui_textbox('p_vol charset', $values[0], 10) . " "
                    . $text{'edit_global_section_vol_charset_default'}
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'vol preset', \%in);
my $select =
    "<select name='p_vol preset'>\n"
  . "<option value='' "
  . ($values[0] eq '' ? "selected" : "") . ">"
  . (
      $values[2]
      ? html_escape($values[1]) . " (" . html_escape($values[2]) . ")"
      : $text{'edit_vol_section_vol_no_preset'}
  ) . "</option>\n";
for my $presetSectionRef (@{$$afpconfRef{volumePresetSections}}) {
    $select .=
        "<option value='"
      . html_escape($$presetSectionRef{name}) . "' "
      . ($values[0] eq $$presetSectionRef{name} ? "selected" : "") . ">"
      . html_escape($$presetSectionRef{name})
      . "</option>\n";
}
$select .= "</select>";
print &ui_table_row($text{'edit_global_section_vol_preset'}, $select);

print &ui_table_end();
print &ui_tabs_end_tab('mode', 'common');

print &ui_tabs_start_tab('mode', 'auth');
print &ui_table_start($text{'edit_global_section_title_table'}, 'width="100%"', 2);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'ad domain', \%in);
print &ui_table_row(
                    $text{'edit_global_section_ad_domain'},
                    &ui_textbox('p_ad domain', $values[0], 40)
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'admin auth user', \%in);
print &ui_table_row(
                    $text{'edit_global_section_admin_auth_user'},
                    &ui_textbox('p_admin auth user', $values[0], 40) . &user_chooser_button("admin auth user")
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'admin group', \%in);
print &ui_table_row(
                    $text{'edit_global_section_admin_group'},
                    &ui_textbox('p_admin group', $values[0], 40) . &group_chooser_button("admin group")
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'force user', \%in);
print &ui_table_row(
                    $text{'edit_global_section_force_user'},
                    &ui_textbox('p_force user', $values[0], 40) . &user_chooser_button("force user")
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'force group', \%in);
print &ui_table_row(
                    $text{'edit_global_section_force_group'},
                    &ui_textbox('p_force group', $values[0], 40) . &group_chooser_button("force group")
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'k5 keytab', \%in);
print &ui_table_row(
                    $text{'edit_global_section_kerberos_keytab'},
                    &ui_filebox('p_k5_keytab', $values[0], 40)
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'k5 service', \%in);
print &ui_table_row(
                    $text{'edit_global_section_kerberos_service'},
                    &ui_textbox('p_k5 service', $values[0], 40)
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'k5 realm', \%in);
print &ui_table_row(
                    $text{'edit_global_section_kerberos_realm'},
                    &ui_textbox('p_k5 realm', $values[0], 40)
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'nt domain', \%in);
print &ui_table_row(
                    $text{'edit_global_section_nt_domain'},
                    &ui_textbox('p_nt domain', $values[0], 40)
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'nt separator', \%in);
print &ui_table_row(
                    $text{'edit_global_section_nt_separator'},
                    &ui_textbox('p_nt separator', $values[0], 40)
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'save password', \%in);
print &ui_table_row(
                    $text{'edit_global_section_save_password'},
                    &build_select(
                                  $afpconfRef, $sectionRef, \%in, 'save password', $text{'edit_undefined'}, 'yes',
                                  'yes',       'no',        'no'
                    )
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'set password', \%in);
print &ui_table_row(
                    $text{'edit_global_section_set_password'},
                    &build_select(
                                  $afpconfRef, $sectionRef, \%in, 'set password', $text{'edit_undefined'}, 'yes',
                                  'yes',       'no',        'no'
                    )
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'passwd file', \%in);
print &ui_table_row(
                    $text{'edit_global_section_passwd_file'},
                    &ui_filebox('p_passwd file', $values[0]) . " "
                    . ($values[2] ? html_escape($values[2]) . ": " . html_escape($values[1]) : '') . "\n"
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'passwd minlen', \%in);
print &ui_table_row(
                    $text{'edit_global_section_passwd_minlen'},
                    "<input name='p_passwd minlen' type='number' value='"
                    . $values[0] . "'>" . " "
                    . $text{'edit_global_section_passwd_minlen_help'}
);

print &ui_table_end();
print &ui_tabs_end_tab('mode', 'auth');

print &ui_tabs_start_tab('mode', 'network');
print &ui_table_start($text{'edit_global_section_title_table'}, 'width="100%"', 2);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'advertise ssh', \%in);
print &ui_table_row(
                    $text{'edit_global_section_advertise_ssh'},
                    &build_select(
                                  $afpconfRef, $sectionRef, \%in, 'advertise ssh', $text{'edit_undefined'}, 'yes',
                                  'yes',       'no',        'no'
                    )
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'afp interfaces', \%in);
print &ui_table_row(
                    $text{'edit_global_section_afp_interfaces'},
                    &ui_textbox('p_afp interfaces', $values[0], 40) . " "
                    . ($values[2] ? html_escape($values[2]) . ": " . html_escape($values[1]) : '') . "\n"
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'afp listen', \%in);
print &ui_table_row(
                    $text{'edit_global_section_afp_listen'},
                    &ui_textbox('p_afp listen', $values[0]) . " " . $text{'edit_global_section_afp_listen_default'}
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'afp port', \%in);
print &ui_table_row(
                    $text{'edit_global_section_afp_port'},
                    &ui_textbox('p_afp port', $values[0], 5) . " "
                    . ($values[2] ? html_escape($values[2]) . ": " . html_escape($values[1]) : '') . "\n"
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'ddp address', \%in);
print &ui_table_row(
                    $text{'edit_global_section_ddp_address'},
                    &ui_textbox('p_ddp address', $values[0]) . " "
                    . ($values[2] ? html_escape($values[2]) . ": " . html_escape($values[1]) : '') . "\n"
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'ddp zone', \%in);
print &ui_table_row(
                    $text{'edit_global_section_ddp_zone'},
                    &ui_textbox('p_ddp zone', $values[0], 40)
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'disconnect time', \%in);
print &ui_table_row(
                    $text{'edit_global_section_disconnect_time'},
                    "<input name='p_disconnect time' type='number' value='"
                    . $values[0] . "'>" . " "
                    . ($values[2] ? html_escape($values[2]) . ": " . html_escape($values[1]) : '') . "\n"
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'dsireadbuf', \%in);
print &ui_table_row(
                    $text{'edit_global_section_dsireadbuf'},
                    "<input name='p_dsireadbuf' type='number' value='"
                    . $values[0] . "'>" . " "
                    . ($values[2] ? html_escape($values[2]) . ": " . html_escape($values[1]) : '') . "\n"
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'fqdn', \%in);
print &ui_table_row(
                    $text{edit_global_section_fqdn},
                    &ui_textbox('p_fqdn', $values[0], 40)
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'max connections', \%in);
print &ui_table_row(
                    $text{'edit_global_section_max_connections'},
                    "<input name='p_max connections' type='number' value='"
                    . $values[0] . "'>" . " "
                    . ($values[2] ? html_escape($values[2]) . ": " . html_escape($values[1]) : '') . "\n"
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'hostname', \%in);
print &ui_table_row(
                    $text{'edit_global_section_hostname'},
                    &ui_textbox('p_hostname', $values[0], 30) . " " . $text{'edit_global_section_hostname_default'}
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'server quantum', \%in);
print &ui_table_row(
                    $text{'edit_global_section_server_quantum'},
                    "<input name='p_server quantum' type='number' value='"
                    . $values[0] . "'>" . " "
                    . ($values[2] ? html_escape($values[2]) . ": " . html_escape($values[1]) : '') . "\n"
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'sleep time', \%in);
print &ui_table_row(
                    $text{'edit_global_section_sleep_time'},
                    "<input name='p_sleep time' type='number' value='"
                    . $values[0] . "'>" . " "
                    . ($values[2] ? html_escape($values[2]) . ": " . html_escape($values[1]) : '') . "\n"
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'tcprcvbuf', \%in);
print &ui_table_row(
                    $text{'edit_global_section_tcprcvbuf'},
                    "<input name='p_tcprcvbuf' type='number' value='" . $values[0] . "'>"
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'tcpsndbuf', \%in);
print &ui_table_row(
                    $text{'edit_global_section_tcpsndbuf'},
                    "<input name='p_tcpsndbuf' type='number' value='" . $values[0] . "'>"
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'recvfile', \%in);
print &ui_table_row(
                    $text{'edit_global_section_recvfile'},
                    &build_select(
                                  $afpconfRef, $sectionRef, \%in, 'recvfile', $text{'edit_undefined'}, 'yes', 'yes',
                                  'no', 'no'
                    )
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'splice size', \%in);
print &ui_table_row(
                    $text{'edit_global_section_splice_size'},
                    "<input name='p_splice size' type='number' value='"
                    . $values[0] . "'>" . " "
                    . ($values[2] ? html_escape($values[2]) . ": " . html_escape($values[1]) : '') . "\n"
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'use sendfile', \%in);
print &ui_table_row(
                    $text{'edit_global_section_use_sendfile'},
                    &build_select(
                                  $afpconfRef, $sectionRef, \%in, 'use sendfile', $text{'edit_undefined'}, 'yes',
                                  'yes',       'no',        'no'
                    )
);

print &ui_table_end();
print &ui_tabs_end_tab('mode', 'network');

print &ui_tabs_start_tab('mode', 'misc');
print &ui_table_start($text{'edit_global_section_title_table'}, 'width="100%"', 2);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'afp read locks', \%in);
print &ui_table_row(
                    $text{'edit_global_section_afp_read_locks'},
                    &build_select(
                                  $afpconfRef, $sectionRef, \%in, 'afp read locks', $text{'edit_undefined'}, 'yes',
                                  'yes',       'no',        'no'
                    )
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'close vol', \%in);
print &ui_table_row(
                    $text{'edit_global_section_close_vol'},
                    &build_select(
                                  $afpconfRef, $sectionRef, \%in, 'close vol', $text{'edit_undefined'}, 'yes', 'yes',
                                  'no', 'no'
                    )
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'dbus daemon', \%in);
print &ui_table_row(
                    $text{'edit_global_section_dbus_daemon'},
                    &ui_filebox('p_dbus daemon', $values[0])
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'dircachesize', \%in);
print &ui_table_row(
                    $text{'edit_global_section_dircachesize'},
                    "<input name='p_dircachesize' type='number' value='"
                    . $values[0] . "'>" . " "
                    . ($values[2] ? html_escape($values[2]) . ": " . html_escape($values[1]) : '') . "\n"
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'extmap file', \%in);
print &ui_table_row(
                    $text{'edit_global_section_extmap_file'},
                    &ui_filebox('p_extmap file', $values[0])
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'force xattr with sticky bit', \%in);
print &ui_table_row(
                    $text{'edit_global_section_force_xattr_with_sticky_bit'},
                    &build_select(
                                  $afpconfRef, $sectionRef, \%in, 'force xattr with sticky bit',
                                  $text{'edit_undefined'}, 'yes', 'yes', 'no', 'no'
                    )
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'guest account', \%in);
print &ui_table_row(
                    $text{'edit_global_section_guest_account'},
                    &ui_textbox('p_guest account', $values[0], 20)
                    . &user_chooser_button('guest account') . " "
                    . ($values[2] ? html_escape($values[2]) . ": " . html_escape($values[1]) : '') . "\n"
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'ignored attributes', \%in);
print &ui_table_row(
                    $text{'edit_global_section_ignored_attributes'},
                    &build_select(
                                  $afpconfRef, $sectionRef, \%in, 'ignored attributes', $text{'edit_undefined'}, 'all',
                                  'all', 'nowrite', 'nowrite', 'nodelete', 'nodelete', 'norename', 'norename'
                    )
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'log microseconds', \%in);
print &ui_table_row(
                    $text{'edit_global_section_log_microseconds'},
                    &build_select(
                                  $afpconfRef, $sectionRef, \%in, 'log microseconds', $text{'edit_undefined'}, 'yes',
                                  'yes',       'no',        'no'
                    )
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'signature', \%in);
print &ui_table_row(
                    $text{'edit_global_section_signature'},
                    &ui_textbox('p_signature', $values[0], 20, undef, 16)
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'solaris share reservations', \%in);
print &ui_table_row(
                    $text{'edit_global_section_solaris_share_reservations'},
                    &build_select(
                                  $afpconfRef, $sectionRef, \%in, 'solaris share reservations',
                                  $text{'edit_undefined'}, 'yes', 'yes', 'no', 'no'
                    )
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'sparql results limit', \%in);
print &ui_table_row(
                    $text{'edit_global_section_sparql_results_limit'},
                    "<input name='p_sparql results limit' type='number' value='"
                    . $values[0] . "'>" . " "
                    . $text{'edit_global_section_sparql_results_limit_default'}
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'spotlight attributes', \%in);
print &ui_table_row(
                    $text{'edit_global_section_spotlight_attributes'},
                    &ui_textbox('p_spotlight attributes', $values[0], 40) . " "
                    . $text{'edit_global_section_spotlight_attributes_default'}
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'spotlight expr', \%in);
print &ui_table_row(
                    $text{'edit_global_section_spotlight_expr'},
                    &build_select(
                                  $afpconfRef, $sectionRef, \%in, 'spotlight expr', $text{'edit_undefined'}, 'yes',
                                  'yes',       'no',        'no'
                    )
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'veto message', \%in);
print &ui_table_row(
                    $text{'edit_global_section_veto_message'},
                    &build_select(
                                  $afpconfRef, $sectionRef, \%in, 'veto message', $text{'edit_undefined'}, 'yes',
                                  'yes',       'no',        'no'
                    )
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'volnamelen', \%in);
print &ui_table_row(
                    $text{'edit_global_section_volnamelen'},
                    "<input name='p_volnamelen' type='number' value='"
                    . $values[0] . "'>" . " "
                    . ($values[2] ? html_escape($values[2]) . ": " . html_escape($values[1]) : '') . "\n"
);

print &ui_table_end();
print &ui_tabs_end_tab('mode', 'misc');

print &ui_tabs_start_tab('mode', 'cnid');
print &ui_table_start($text{'edit_global_section_title_table'}, 'width="100%"', 2);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'vol dbpath', \%in);
print &ui_table_row(
                    $text{'edit_global_section_vol_dbpath'},
                    &ui_filebox('p_vol_dbpath', $values[0], 40, undef, undef, undef, 1)
                    . ($values[2] ? html_escape($values[2]) . ": " . html_escape($values[1]) : '')
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'cnid listen', \%in);
print &ui_table_row(
                    $text{'edit_global_section_cnid_listen'},
                    &ui_textbox('p_cnid listen', $values[0], 20) . " "
                    . ($values[2] ? html_escape($values[2]) . ": " . html_escape($values[1]) : '') . "\n"
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'cnid server', \%in);
print &ui_table_row(
                    $text{'edit_global_section_cnid_server'},
                    &ui_textbox('p_cnid server', $values[0], 20) . " "
                    . ($values[2] ? html_escape($values[2]) . ": " . html_escape($values[1]) : '') . "\n"
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'vol dbnest', \%in);
print &ui_table_row(
                    $text{'edit_global_section_vol_dbnest'},
                    &build_select(
                                  $afpconfRef, $sectionRef, \%in, 'vol dbnest', $text{'edit_undefined'}, 'yes', 'yes',
                                  'no', 'no'
                    )
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'cnid mysql host', \%in);
print &ui_table_row(
                    $text{'edit_global_section_cnid_mysql_host'},
                    &ui_textbox('p_cnid mysql host', $values[0], 40)
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'cnid mysql user', \%in);
print &ui_table_row(
                    $text{'edit_global_section_cnid_mysql_user'},
                    &ui_textbox('p_cnid mysql user', $values[0], 40)
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'cnid mysql pw', \%in);
print &ui_table_row(
                    $text{'edit_global_section_cnid_mysql_pw'},
                    &ui_textbox('p_cnid mysql pw', $values[0], 40)
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'cnid mysql db', \%in);
print &ui_table_row(
                    $text{'edit_global_section_cnid_mysql_db'},
                    &ui_textbox('p_cnid mysql db', $values[0], 40)
);

print &ui_table_end();
print &ui_tabs_end_tab('mode', 'cnid');

print &ui_tabs_start_tab('mode', 'acl');
print &ui_table_start($text{'edit_global_section_title_table'}, 'width="100%"', 2);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'chmod request', \%in);
print &ui_table_row(
                    $text{'edit_global_section_chmod_request'},
                    &build_select(
                                  $afpconfRef, $sectionRef, \%in, 'chmod request', $text{'edit_undefined'}, 'preserve',
                                  'preserve',  'ignore',    'ignore', 'simple',    'simple'
                    )
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'map acls', \%in);
print &ui_table_row(
                    $text{'edit_global_section_map_acls'},
                    &build_select(
                                  $afpconfRef, $sectionRef, \%in,   'map acls', $text{'edit_undefined'}, 'none', 'none',
                                  'rights',    'rights',    'mode', 'mode'
                    )
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'ldap auth method', \%in);
print &ui_table_row(
                    $text{'edit_global_section_ldap_auth_method'},
                    &build_select(
                                  $afpconfRef, $sectionRef, \%in, 'ldap auth method', $text{'edit_undefined'}, 'none',
                                  'none',      'simple',    'simple'
                    )
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'ldap auth dn', \%in);
print &ui_table_row(
                    $text{'edit_global_section_ldap_auth_dn'},
                    &ui_textbox('p_ldap auth dn', $values[0], 40)
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'ldap auth pw', \%in);
print &ui_table_row(
                    $text{'edit_global_section_ldap_auth_pw'},
                    &ui_textbox('p_ldap auth pw', $values[0], 40)
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'ldap uri', \%in);
print &ui_table_row(
                    $text{'edit_global_section_ldap_uri'},
                    &ui_textbox('p_ldap uri', $values[0], 40) . " " . $text{'edit_global_section_ldap_uri_help'}
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'ldap userbase', \%in);
print &ui_table_row(
                    $text{'edit_global_section_ldap_userbase'},
                    &ui_textbox('p_ldap userbase', $values[0], 40)
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'ldap userscope', \%in);
print &ui_table_row(
                    $text{'edit_global_section_ldap_userscope'},
                    &build_select(
                                  $afpconfRef, $sectionRef, \%in,  'ldap userscope', $text{'edit_undefined'}, 'base',
                                  'base',      'one',       'one', 'sub',            'sub'
                    )
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'ldap groupbase', \%in);
print &ui_table_row(
                    $text{'edit_global_section_ldap_groupbase'},
                    &ui_textbox('p_ldap groupbase', $values[0], 40)
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'ldap groupscope', \%in);
print &ui_table_row(
                    $text{'edit_global_section_ldap_groupscope'},
                    &build_select(
                                  $afpconfRef, $sectionRef, \%in,  'ldap groupscope', $text{'edit_undefined'}, 'base',
                                  'base',      'one',       'one', 'sub',             'sub'
                    )
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'ldap uuid attr', \%in);
print &ui_table_row(
                    $text{'edit_global_section_ldap_uuid_attr'},
                    &ui_textbox('p_ldap uuid attr', $values[0], 40)
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'ldap name attr', \%in);
print &ui_table_row(
                    $text{'edit_global_section_ldap_name_attr'},
                    &ui_textbox('p_ldap name attr', $values[0], 40)
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'ldap group attr', \%in);
print &ui_table_row(
                    $text{'edit_global_section_ldap_group_attr'},
                    &ui_textbox('p_ldap group attr', $values[0], 40)
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'ldap uuid string', \%in);
print &ui_table_row(
                    $text{'edit_global_section_ldap_uuid_string'},
                    &ui_textbox('p_ldap uuid string', $values[0], 40)
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'ldap uuid encoding', \%in);
print &ui_table_row(
                    $text{'edit_global_section_ldap_uuid_encoding'},
                    &build_select(
                                  $afpconfRef, $sectionRef, \%in,      'ldap uuid encoding', $text{'edit_undefined'},
                                  'string',    'string',    'ms-guid', 'ms-guid'
                    )
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'ldap user filter', \%in);
print &ui_table_row(
                    $text{'edit_global_section_ldap_user_filter'},
                    &ui_textbox('p_ldap user filter', $values[0], 40)
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'ldap group filter', \%in);
print &ui_table_row(
                    $text{'edit_global_section_ldap_group_filter'},
                    &ui_textbox('p_ldap group filter', $values[0], 40)
);

print &ui_table_end();
print &ui_tabs_end_tab('mode', 'acl');

print &ui_tabs_start_tab('mode', 'fce');
print &ui_table_start($text{'edit_global_section_title_table'}, 'width="100%"', 2);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'fce listener', \%in);
print &ui_table_row(
                    $text{'edit_global_section_fce_listener'},
                    &ui_textbox('p_fce listener', $values[0], 40) . " "
                    . ($values[2] ? html_escape($values[2]) . ": " . html_escape($values[1]) : '') . "\n"
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'fce version', \%in);
print &ui_table_row(
                    $text{'edit_global_section_fce_version'},
                    &build_select(
                                  $afpconfRef, $sectionRef, \%in, 'fce version', $text{'edit_undefined'}, '1', '1',
                                  '2', '2'
                    )
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'fce events', \%in);
print &ui_table_row(
                    $text{'edit_global_section_fce_events'},
                    &ui_textbox('p_fce events', $values[0], 40)
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'fce coalesce', \%in);
print &ui_table_row(
                    $text{'edit_global_section_fce_coalesce'},
                    &build_select(
                                  $afpconfRef, $sectionRef, \%in,     'fce coalesce', $text{'edit_undefined'}, 'all',
                                  'all',       'delete',    'delete', 'create',       'create'
                    )
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'fce holdfmod', \%in);
print &ui_table_row(
                    $text{'edit_global_section_fce_holdfmod'},
                    "<input name='p_fce holdfmod' type='number' value='"
                    . $values[0] . "'>" . " "
                    . ($values[2] ? html_escape($values[2]) . ": " . html_escape($values[1]) : '') . "\n"
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'fce sendwait', \%in);
print &ui_table_row(
                    $text{'edit_global_section_fce_sendwait'},
                    "<input name='p_fce sendwait' type='number' min='0' max='999' value='" . $values[0] . "'>"
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'fce ignore names', \%in);
print &ui_table_row(
                    $text{'edit_global_section_fce_ignore_names'},
                    &ui_textbox('p_fce ignore names', $values[0], 40) . " "
                    . ($values[2] ? html_escape($values[2]) . ": " . html_escape($values[1]) : '') . "\n"
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'fce ignore directories', \%in);
print &ui_table_row(
                    $text{'edit_global_section_fce_ignore_directories'},
                    &ui_textbox('p_fce ignore directories', $values[0], 40)
);

@values = get_parameter_of_section($afpconfRef, $sectionRef, 'fce notify script', \%in);
print &ui_table_row(
                    $text{'edit_global_section_fce_notify_script'},
                    &ui_filebox('p_fce notify script', $values[0])
);

print &ui_table_end();
print &ui_tabs_end_tab('mode', 'fce');

print &ui_tabs_end();

print &ui_form_end([[undef, $text{'save_button_title'}, 0, undef]]);

ui_print_footer("index.cgi?tab=general", $text{'edit_return'});
