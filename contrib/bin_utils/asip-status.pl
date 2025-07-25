#!@PERL@
#
# asip-status - send DSIGetStatus to an AppleShare IP file server (aka
#               ASIP, aka AFP over TCP port 548).  A returned UAM of
#               "No User Authen" means that the server supports guest access.
#
# (c) 1997-2001 James W. Abendschan <jwa@jammed.com>
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
# see also:
#   - http://developer.apple.com/techpubs/macos8/NetworkCommSvcs/AppleShare/
#   - http://www2.opendoor.com/asip/   (excellent Mac sharing / security site)
#
# todo: log in as guest & get a list of shares
#

#
# This edition is a part of netatalk @netatalk_version@.
#

use strict;
# sucks because Timeout doesn't
use IO::Socket::IP;

my ($arg);
my ($hostport);
my ($host);
my ($port);

while ($arg = shift @ARGV) {
    $main::ipv       = AF_INET  if ($arg eq "-4");
    $main::ipv       = AF_INET6 if ($arg eq "-6");
    $main::show_icon = 1        if ($arg eq "-i");
    $main::debug     = 1        if ($arg eq "-d");
    $main::hexdump   = 1        if ($arg eq "-x");
    $main::showver   = 1        if ($arg eq "-v");
    $main::showver   = 1        if ($arg eq "-version");
    $main::showver   = 1        if ($arg eq "--version");
    $port            = $arg     if (($arg !~ /^-/) && ($hostport));
    $hostport        = $arg     if (($arg !~ /^-/) && !($hostport));
}

if ($main::showver == 1) {
    print "$0\n";
    print "Original edition: 7 May 1997 \(v1.0\) James W. Abendschan\n";
    print "This edition is a part of Netatalk @netatalk_version@\n";
    exit(-1);
}

if ($hostport eq "") {
    print "usage: $0 [-4|-6] [-d] [-i] [-x] hostname [port]\n";
    print "       $0 [-4|-6] [-d] [-i] [-x] hostname:port\n";
    print "       $0 -v|-version|--version\n";
    print "Queries AFP servers for their capabilities.\n";
    print "  -4: Use IPv4 only.\n";
    print "  -6: Use IPv6 only.\n";
    print "  -d: Enable debug output.\n";
    print "  -i: Show icon if it exists.\n";
    print "  -x: Enable hex dump output.\n";
    print "  -v,-version,--version: Show version.\n";
    exit(-1);
}

($host, $port) = parse_hostport($hostport, $port);

my ($packet) = build_packet();
my ($code)   = sendpacket($host, $port, $packet);
exit $code;

sub parse_hostport {
    my ($hostport, $port) = @_;
    my ($host);

    if (($port) && ($hostport =~ /\[\S+\]/)) {
        # [fd01:1::1] 10548
        $host = $hostport;
    } elsif (($port) && ($hostport =~ /:/)) {
        # fd01:1::1 10548
        $host = "\[" . $hostport . "\]";
    } elsif ($port) {
        # myserver 10548 / 192.168.0.1 10548
        $host = $hostport;
    } elsif ($hostport =~ /\[(\S+)\]:(\S+)/) {
        # [fd01:1::1]:10548
        $host = "\[" . $1 . "\]";
        $port = $2;
    } elsif ($hostport =~ /\[\S+\]/) {
        # [fd01:1::1]
        $host = $hostport;
        $port = "548";
    } elsif ((() = $hostport =~ m/:/g) >= 2) {
        # fd01:1::1
        $host = "\[" . $hostport . "\]";
        $port = "548";
    } else {
        # myserver:10548 / myserver / 192.168.0.1:10548 / 192.168.0.1
        ($host, $port) = split(/\:/, $hostport);
        $port = "548" if ($port eq "");
    }

    return ($host, $port);
}

sub build_packet {
    my (@packet) = (
                    # 0- request, 1-reply
                    0x00,
                    # 3- DSIGetStatus
                    0x03,
                    # request ID
                    0xde, 0xad, 0x00,
                    # data field
                    0x00, 0x00, 0x00, 0x00,
                    # length of data stream header
                    0x00, 0x00, 0x00, 0x00,
                    # reserved
                    0x00, 0x00, 0x00, 0x00
    );

    my ($packet) = pack("C*", @packet);

    return $packet;

}

sub sendpacket {
    my ($host, $port, $packet) = @_;
    my ($b, $buf);

    print "opening $host:$port\n" if ($main::debug);

    my ($asip_sock) =
      IO::Socket::IP->new(
                          Domain   => $main::ipv,
                          PeerAddr => $host,
                          PeerPort => $port,
                          Proto    => 'tcp',
                          Type     => SOCK_STREAM,
                          Timeout  => 10
      ) || die "connect to $host failure: $!";
    $asip_sock->autoflush(1);

    print "sending packet\n" if ($main::debug);

    my ($count) = syswrite($asip_sock, $packet, length($packet));

    if ($count != length($packet)) {
        print "only wrote $count of " . length($packet) . " bytes?\n";
        exit(-1);
    }

    # reply can span multiple packets

    print "sysread: " if ($main::debug);
    while (sysread($asip_sock, $b, 256)) {
        $buf .= $b;
        print "." if ($main::debug);
    }

    close($asip_sock);

    print " read " . length($buf) . " bytes\n" if ($main::debug);

    if (length($buf) == 0) {
        print "empty reply packet?\n";
        return -2;
    } else {
        print "AFP reply from $host:$port";
        if ($asip_sock->sockdomain == PF_INET) {
            print " via IPv4\n";
        } elsif ($asip_sock->sockdomain == PF_INET6) {
            print " via IPv6\n";
        } else {
            print " via unknown IP version\n";
        }
        return (parse_packet($buf));
    }
}

sub parse_packet {
    my ($buf) = shift @_;
    my (@packet);
    my ($i);

    hexdump($buf) if ($main::hexdump);

    for ($i = 0 ; $i < length($buf) ; $i++) {
        push(@packet, substr($buf, $i, 1));
    }

    my ($flags) = unpack("C", @packet[0]);
    my ($cmd)   = unpack("C", @packet[1]);

    my ($request_id) = unpack("n", @packet[2] . @packet[3]);
    print "Flags: $flags  Cmd: $cmd  ID: $request_id\n";

    print getasipsrv("flags", $flags) . ": " . getasipsrv("command", $cmd) . "\n";
    print "Request ID: $request_id\n";

    print "** Request ID didn't match what we sent!\n" if ($request_id != 0xdead);

    # "Error Code / Enclosed Data Offset"
    # I have never seen this be non-zero ..

    my ($edo) = unpack("N2", @packet[4] . @packet[5] . @packet[6] . @packet[7]);
    print "** Wow, a non-zero Error/Enclosed Data Offset: $edo\n" if ($edo);

    # "Total Data Length"

    my ($datalen) = unpack("N2", @packet[8] . @packet[9] . @packet[10] . @packet[11]);

    print "Total data length: $datalen\n" if ($main::debug);

    # "Reserved Field"

    my ($reserved) = unpack("N2", @packet[12] . @packet[13] . @packet[14] . @packet[15]);

    print "Reserved field: $reserved\n" if ($reserved);

    if ($cmd != 3) {
        print "I can only parse packets of reply-type DSIGetStatus (3)\n";
        print "This is reply-type " . getasipsrv("command", $cmd) . "\n";
    }
    if ($datalen == 0) {
        print "No data in packet?\n";
    }
    if (($datalen > 0) && ($cmd == 3)) {
        my (@AFPpacket) = @packet[($edo + 16) .. ($edo + 16 + $datalen)];
        return (parse_FPGetSrvrInfo(@AFPpacket));
    } else {
        print "I don't know how to parse this type of packet.\n";
        return (2);
    }
}

sub parse_FPGetSrvrInfo() {
    my (@packet) = @_;
    my ($i);

    my ($machinetype_offset) = unpack("n", @packet[0] . @packet[1]);
    print "Machine type offset in packet: $machinetype_offset\n" if ($main::debug);
    my ($machinetype) = extract(1, $machinetype_offset, @packet);
    print "Machine type: $machinetype\n";

    my ($afpversioncount_offset) = unpack("n", @packet[2] . @packet[3]);
    print "AFPversion count offset: $afpversioncount_offset\n" if ($main::debug);
    my (@afpversions) = extract(0, $afpversioncount_offset, @packet);
    print "AFP versions: " . join(",", @afpversions) . "\n";

    my ($uamcount_offset) = unpack("n", @packet[4] . @packet[5]);
    print "UAMcount offset: $uamcount_offset\n" if ($main::debug);
    my (@uams) = extract(0, $uamcount_offset, @packet);
    print "UAMs: " . join(",", @uams) . "\n";

    my ($allow_guest) = 0;
    $allow_guest = 1 if (grep(/No User Authen/, @uams));

    # it would be cute to see the icon.

    my ($icon_offset) = unpack("n", @packet[6] . @packet[7]);
    print "Volume Icon & Mask offset: $icon_offset\n" if ($main::debug);
    print "Volume Icon & Mask: ";
    if ($icon_offset) {
        print "Yes\n";
    } else {
        print "No\n";
    }

    my ($flags) = unpack("n", @packet[8] . @packet[9]);
    my (@flags) = parse_afp_flags($flags);

    print "Flags: ";
    print "$flags" if ($main::debug);
    print "\n";
    print join("\n", @flags) . "\n";

    # server name starts at offset+10, length byte first.

    my ($servername_len) = unpack("C1", @packet[10]);
    my ($servername)     = join("", @packet[11 .. (11 + $servername_len - 1)]);
    print "Server name length: $servername_len\n" if ($main::debug);
    print "Server name: $servername\n";

    my ($offset) = 11 + $servername_len;

    # quietly ++ the $offset to account for the padding that happens
    # in the reply packet if the field names don't align on an even boundary

    $offset++ if ($servername_len % 2 == 0);

    print "New offset: $offset\n" if ($main::debug);

    my ($signature_offset) = unpack("n2", @packet[$offset] . @packet[$offset + 1]);
    print "Signature offset: $signature_offset\n" if ($main::debug);
    if ($signature_offset) {
        my ($signature) = join("", @packet[$signature_offset .. $signature_offset + 15]);

        print "Signature:\n";
        hexdump($signature);
    }

    my ($network_address_count_offset) = unpack("n2", @packet[$offset + 2] . @packet[$offset + 3]);
    print "Network address count offset: $network_address_count_offset\n" if ($main::debug);

    extract_network_address($network_address_count_offset, @packet);

    $offset += 4;
    # Supports directory services
    if ($flags & (1 << 8)) {
        my ($directory_service_offset) = unpack("n2", @packet[$offset] . @packet[$offset + 1]);
        print "Directory service offset: $directory_service_offset\n" if ($main::debug);
        if ($directory_service_offset) {
            my (@dirsvcs) = extract(0, $directory_service_offset, @packet);
            while (@dirsvcs) {
                printf "Directory Service: %s\n", shift @dirsvcs;
            }
        }
        $offset += 2;
    }

    # Supports UTF8 servername
    if ($flags & (1 << 9)) {
        my ($utf8_name_offset) = unpack("n2", @packet[$offset] . @packet[$offset + 1]);
        print "UTF8 name offset: $utf8_name_offset\n" if ($main::debug);
        if ($utf8_name_offset) {
            my ($utf8name) = extract(1, $utf8_name_offset + 1, @packet);
            print "UTF8 Servername: $utf8name\n";
        }
        $offset += 2;
    }

    # Undocumented Bit12
    if ($flags & (1 << 12)) {
        # looks like a SPNEGNO negTokenInit blob with the extensions from MS-SPNG
        # <http://download.microsoft.com/download/9/5/E/95EF66AF-9026-4BB0-A41D-A4F81802D92C/[MS-SPNG].pdf>
        # <https://tools.ietf.org/html/rfc4178#section-4.2.1>
        my ($gss_uam_spnego_blob_offset) = unpack("n2", @packet[$offset] . @packet[$offset + 1]);
        print "GSS-UAM SPNEGO blob offset: $gss_uam_spnego_blob_offset\n" if ($main::debug);
        if ($gss_uam_spnego_blob_offset) {
            # the leading 4-bytes mean length maybe...
            my ($gss_uam_spnego_blob_len) =
              unpack(
                     "N2",
                     @packet[$gss_uam_spnego_blob_offset]
                     . @packet[$gss_uam_spnego_blob_offset + 1]
                     . @packet[$gss_uam_spnego_blob_offset + 2]
                     . @packet[$gss_uam_spnego_blob_offset + 3]
              );
            my ($gss_uam_spnego_blob) =
              join(
                   "",
                   @packet[
                   $gss_uam_spnego_blob_offset + 4 .. $gss_uam_spnego_blob_offset + 4 + $gss_uam_spnego_blob_len - 1
                   ]
              );
            print "GSS-UAM SPNEGO blob length: $gss_uam_spnego_blob_len\n" if ($main::debug);
            print "GSS-UAM SPNEGO blob (Undocumented):\n";
            hexdump($gss_uam_spnego_blob);
        }
    }

    draw_icon($icon_offset, @packet) if ($main::show_icon && $icon_offset);

    return $allow_guest;
}

# getsrvbyname .. sorta ..

sub getasipsrv {
    my ($what, $code) = @_;

    if ($what eq "flags") {
        return "Request" if ($code == 0);
        return "Reply"   if ($code == 1);
    }

    if ($what eq "command") {
        return "DSICloseSession" if ($code == 1);
        return "DSICommand"      if ($code == 2);
        return "DSIGetStatus"    if ($code == 3);
        return "DSIOpenSession"  if ($code == 4);
        return "DSITickle"       if ($code == 5);
        return "DSIWrite"        if ($code == 6);
        return "DSIAttention"    if ($code == 7);
    }
    return "[$what/$code] unknown";
}

# return "counted" data at @packet[$offset]
# when called with a zero as the first argument, this will
# look in the packet for the count.  Otherwise, it will
# assume I know what I'm doing.  (hah, what a foolish function..)

sub extract {
    my ($count, $offset, @packet) = @_;
    my ($i,     $j);
    my (@items, $data);
    my ($hack);

    if ($count == 0) {
        ($count) = unpack("C", @packet[$offset]);
        return if ($count == 0);
        $offset++;
    } else {
        $hack = 1;
    }
    #print ">> extracting $count items from offset $offset\n";
    for ($i = 0 ; $i < $count ; $i++) {
        #print "Working on count $i\n";
        my ($len) = unpack("C1", @packet[$offset]);
        $data = join("", @packet[$offset + 1 .. $offset + $len]);
        #print "$i. [$data] ($len)\n";
        push(@items, $data);
        $offset = $offset + $len + 1;
        #print "new offset is $offset\n";
    }
    return $data if ($hack);
    return @items;
}

sub draw_icon {
    my ($offset, @packet) = @_;
    my ($cols);
    my ($i, $j);

    # icons are 32x32 bitmaps; 128 byte icon + 128 byte mask
    # to show the mask, change 128 to 256.

    for ($i = 0 ; $i < 128 ; $i++) {
        my ($c)   = @packet[$i + $offset];
        my ($bin) = unpack("B*", $c);

        for ($j = 0 ; $j < 8 ; $j++) {
            if (substr($bin, $j, 1)) {
                print "#";
            } else {
                print " ";
            }
        }
        $cols++;
        if ($cols == 4) {
            $cols = 0;
            print "\n";
        }

    }
    print "\n";
}

sub parse_afp_flags {
    my ($flags) = shift @_;
    my (@flags);

    # $flags is a 16 bit little-endian number

    push(@flags, "    SupportsCopyFile")                                  if ($flags & (1 << 0));
    push(@flags, "    SupportsChgPwd")                                    if ($flags & (1 << 1));
    push(@flags, "    DontAllowSavePwd")                                  if ($flags & (1 << 2));
    push(@flags, "    SupportsServerMessages")                            if ($flags & (1 << 3));
    push(@flags, "    SupportsServerSignature")                           if ($flags & (1 << 4));
    push(@flags, "    SupportsTCP/IP")                                    if ($flags & (1 << 5));
    push(@flags, "    SupportsSrvrNotifications")                         if ($flags & (1 << 6));
    push(@flags, "    SupportsReconnect")                                 if ($flags & (1 << 7));
    push(@flags, "    SupportsOpenDirectory")                             if ($flags & (1 << 8));
    push(@flags, "    SupportsUTF8Servername")                            if ($flags & (1 << 9));
    push(@flags, "    SupportsUUIDs")                                     if ($flags & (1 << 10));
    push(@flags, "    SupportsExtSleep")                                  if ($flags & (1 << 11));
    push(@flags, "    Undocumented Bit12 (Supports GSS-UAM SPNEGO blob)") if ($flags & (1 << 12));
    push(@flags, "    Undocumented Bit13")                                if ($flags & (1 << 13));
    push(@flags, "    Undocumented Bit14")                                if ($flags & (1 << 14));
    push(@flags, "    SupportsSuperClient")                               if ($flags & (1 << 15));

    return @flags;
}

sub hexdump {
    my ($buf) = @_;
    my ($p, $c, $pc, $str);
    my ($i);

    for ($i = 0 ; $i < length($buf) ; $i++) {
        $p = substr($buf, $i, 1);
        $c = ord($p);
        printf "%.2x ", $c;
        $pc++;
        if (($c > 31) && ($c < 127)) {
            $str .= $p;
        } else {
            $str .= ".";
        }
        if ($pc == 16) {
            print " $str\n";
            undef $str;
            $pc = 0;
        }
    }
    print "   " x (16 - $pc);
    print " $str \n";
}

sub extract_network_address {
    my ($offset, @packet) = @_;
    my ($count);
    my ($i) = 0;
    my ($data);

    # get the number of addresses
    ($count) = unpack("C", @packet[$offset]);
    return if ($count == 0);
    $offset++;

    #print "\n>> extracting $count items from offset $offset\n";
    for ($i = 0 ; $i < $count ; $i++) {
        #print "Working on count $i\n";
        my ($len) = unpack("C1", @packet[$offset]);
        #printf "len:  %u\n",$len;
        my ($type) = unpack("C1", @packet[$offset + 1]);
        #printf "type: %u\n",$type;
        $data = join("", @packet[$offset + 2 .. $offset + $len - 1]);
        #print "$i. [$data] ($len)\n";
        $offset = $offset + $len;
        #print "new offset is $offset\n";

        # 1st byte is 'tag'
        # 1 - IP address; 4 bytes
        # 2 - IP address (4) + port (2)
        # 3 - DDP (2 bytes net, 1 byte node, 1 byte socket)
        # 4 - DNS address
        # 5 - IP address (4) + port (2), for SSH tunnel
        # 6 - IPV6 address (16)
        # 7 - IPV6 address (16) + port (2)

        my (@nap) = unpack("C*", $data);

        if ($type == 1) {
            # quad
            my ($ip) = sprintf "%d.%d.%d.%d (IPv4 address)",
              $nap[0], $nap[1], @nap[2], @nap[3];

            print "Network address: $ip\n";
        } elsif ($type == 2) {
            # quad+port
            my ($ipport) = sprintf "%d.%d.%d.%d:%d",
              @nap[0], @nap[1], @nap[2], @nap[3], (@nap[4] * 256 + @nap[5]);
            print "Network address: $ipport (IPv4 address and port)\n";
        } elsif ($type == 3) {
            printf "Network address: %d.%d (ddp address)\n",
              (@nap[0] * 256) + @nap[1], @nap[2];
        } elsif ($type == 4) {
            print "Network address: $data (DNS address)\n";
        } elsif ($type == 5) {
            # according to the specs this should be an IP address
            # however, OSX Server uses the FQDN instead
            print "Network address: $data (SSH tunnel address)\n";
        } elsif ($type == 6 || $type == 7) {
            print "Network address: \[";
            my ($j) = 0;
            for ($j = 0 ; $j <= 13 ; $j = $j + 2) {
                printf("%.2x%.2x:", @nap[$j], @nap[$j + 1]);
            }
            printf("%.2x%.2x\]", @nap[14], @nap[15]);
            if ($type == 7) {
                printf(":%d", (@nap[16] * 256) + @nap[17]);
                print " (IPv6 address + port)\n";
            } else {
                print " (IPv6 address)\n";
            }
        } else {
            printf "unsupported address type: %u\n", $type;
        }

    }
}
