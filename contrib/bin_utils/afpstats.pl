#!@PERL@
#
# AFP Statistics over D-Bus
#
# (c) 2024 Daniel Markstedt <daniel@mindani.net>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#

use strict;
use warnings;
use Net::DBus;

sub main {
    my $bus = Net::DBus->system();

    eval {
        my $service = $bus->get_service("org.netatalk.AFPStats");
        my $remote_object = $service->get_object(
                                                 "/org/netatalk/AFPStats",
                                                 "org.netatalk.AFPStats"
        );

        print "Connected user   PID      Login time        State          Mounted volumes\n";

        foreach my $user (@{$remote_object->GetUsers}) {
            if ($user =~ /name: ([^,]+), pid: ([^,]+), logintime: ([^,]+), state: ([^,]+), volumes: (.+)/) {
                my ($name, $pid, $logintime, $state, $volume) = ($1, $2, $3, $4, $5);
                printf "%-17s%-9s%-18s%-15s%s\n", $name, $pid, $logintime, $state, $volume;
            } else {
                print "WARNING Unexpected output. This is probably a bug:\n" . $user . "\n";
            }
        }
    };
    if ($@) {
        print "Error: $@\n";
        exit 1;
    }
}

main();
