#!@PERL@
#
# AFP Statistics over a Unix domain socket
#
# (c) 2024-2026 Daniel Markstedt <daniel@mindani.net>
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
use File::Basename;
use IO::Socket::UNIX;

my $DEFAULT_SOCKET = '@localstatedir@/netatalk/afpstats.sock';

sub usage {
    printf("Usage: %s [-h client-address] [-s socket] [-v]\n", basename($0));
    exit 1;
}

sub display_name {
    my ($name) = @_;

    if ($name =~ /^uid=([0-9]+)$/) {
        my @pw = getpwuid($1);
        return $pw[0] if defined $pw[0];
    }

    return $name;
}

sub main {
    my $hostname_filter;
    my $socket_path = $DEFAULT_SOCKET;

    while (my $arg = shift @ARGV) {
        if ($arg =~ /^(-v|--version)$/) {
            printf('%s (Netatalk @netatalk_version@)' . "\n", basename($0));
            exit 1;
        } elsif ($arg =~ /^(-h|--hostname)$/) {
            $hostname_filter = shift @ARGV;
            usage() unless defined $hostname_filter;
        } elsif ($arg =~ /^(-s|--socket)$/) {
            $socket_path = shift @ARGV;
            usage() unless defined $socket_path;
        } else {
            usage();
        }
    }

    my $sock = IO::Socket::UNIX->new(
                                     Peer => $socket_path,
                                     Type => SOCK_STREAM,
    );

    if (!$sock) {
        print "Error: cannot connect to $socket_path: $!\n";
        exit 1;
    }

    my @header = ('Connected user', 'PID', 'Login time', 'State', 'Protocol', 'Client address', 'Mounted volumes');
    my @rows;

    while (defined(my $user = <$sock>)) {
        chomp $user;

        if ($user =~
/^name: ([^,]+), pid: ([^,]+), logintime: ([^,]+), state: ([^,]+), protocol: ([^,]+), volumes: (.*), hostname: (.+)/
        ) {
            my ($name, $pid, $logintime, $state, $protocol, $volume, $hostname) = ($1, $2, $3, $4, $5, $6, $7);
            $name = display_name($name);
            if (defined $hostname_filter && $hostname ne $hostname_filter) {
                next;
            }
            push @rows, [$name, $pid, $logintime, $state, $protocol, $hostname, $volume];
        } else {
            print "WARNING Unexpected output. This is probably a bug:\n" . $user . "\n";
        }
    }

    close $sock;

    # Compute column widths from header + data so long hostnames or volume
    # lists don't bleed into the next column.
    my @widths = map {length} @header;
    for my $row (@rows) {
        for my $i (0 .. $#widths) {
            my $len = length($row->[$i]);
            $widths[$i] = $len if $len > $widths[$i];
        }
    }

    # Two-space gap between columns; the last column is left unpadded so
    # we don't emit trailing whitespace.
    my @left = @widths[0 .. $#widths - 1];
    my $fmt  = join('  ', map {"%-${_}s"} @left) . '  %s' . "\n";
    printf $fmt, @header;
    printf $fmt, @$_ for @rows;
}

main();
