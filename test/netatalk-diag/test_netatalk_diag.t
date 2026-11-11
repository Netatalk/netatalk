#!/usr/bin/perl
#
# Copyright (c) 2026 Andy Lemin (andylemin)
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
# Unit tests of netatalk_diag.pl: synthetic captures, counters,
# configurations and report folders with known contents.
#
# usage: perl test_netatalk_diag.t PATH/TO/netatalk_diag.pl

use strict;
use warnings;
use File::Spec ();
use File::Temp ();
use List::Util qw(min);
use POSIX      ();
use Test::More;
use Time::HiRes ();

my $helper = shift @ARGV or BAIL_OUT('the path of netatalk_diag.pl is needed');
require File::Spec->rel2abs($helper);

my ($CLIENT, $SERVER) = (pack('C4', 10, 0, 0, 2), pack('C4', 10, 0, 0, 1));
my ($CPORT, $SPORT) = (50000, 548);

sub slurp {
    my ($path) = @_;
    open(my $fh, '<:raw', $path) or return '';
    local $/;
    return scalar <$fh>;
}

sub spew {
    my ($path, $bytes) = @_;
    open(my $fh, '>:raw', $path) or die "$path: $!";
    print {$fh} $bytes;
    close $fh;
    return;
}

sub has {
    my ($text, @parts) = @_;
    return !grep { index($text, $_) < 0 } @parts;
}

# a UTF-8 AFP path (type 3) from bytes with NUL separators
sub path3 {
    my ($names) = @_;
    return pack('C N n', 3, 0x08000103, length $names) . $names;
}

sub getfiledirparms {
    my ($did, $names) = @_;
    return pack('C C n N n n', 34, 0, 1, $did, 0x0001, 0x0100) . path3($names);
}

sub enumext2 {
    my ($did) = @_;
    return pack('C C n N n n n N N', 68, 0, 1, $did, 0x0001, 0x0100, 1000, 1, 65536) . path3('');
}

sub openfork {
    my ($did, $names, $rsrc) = @_;
    return pack('C C n N n n', 26, $rsrc ? 0x80 : 0, 1, $did, 0x0200, 1) . path3($names);
}

sub be64 {
    my ($v) = @_;
    return pack('N N', int($v / 4294967296), $v % 4294967296);
}

sub readext {
    my ($refnum, $offset, $count) = @_;
    return pack('C C n', 60, 0, $refnum) . be64($offset) . be64($count);
}

sub dsi {
    my ($flags, $cmd, $reqid, $payload, $code) = @_;
    return pack('C C n l> N N', $flags, $cmd, $reqid, $code // 0, length $payload, 0) . $payload;
}

sub frame {
    my ($src, $dst, $sport, $dport, $seq, $payload, $flags, $ack, $window, $opts) = @_;
    ($flags, $ack, $window, $opts) = ($flags // 0x18, $ack // 0, $window // 65535, $opts // '');
    my $tcp =
      pack('n n N N C C n n n', $sport, $dport, $seq, $ack, int((20 + length $opts) / 4) << 4, $flags, $window, 0, 0)
      . $opts;
    my $ip = pack('C C n n n C C n a4 a4', 0x45, 0, 20 + length($tcp) + length($payload), 0, 0, 64, 6, 0, $src, $dst);
    return ("\0" x 12) . "\x08\x00" . $ip . $tcp . $payload;
}

sub write_pcap {
    my ($path, $recs) = @_;
    my $pcap = pack('V v v l< V V V', 0xa1b2c3d4, 2, 4, 0, 0, 262144, 1);
    for my $r (sort { $a->[0] <=> $b->[0] } @{$recs}) {
        my ($ts, $data) = @{$r};
        my $sec = int $ts;
        $pcap .= pack('V V V V', $sec, sprintf('%.0f', ($ts - $sec) * 1e6), length $data, length $data) . $data;
    }
    spew($path, $pcap);
    return;
}

# both directions of one connection, with sequence and ACK numbers
sub capture {
    return {recs => [], cseq => 1000, sseq => 9000};
}

sub handshake {
    my ($c, $t, $cws, $sws) = @_;
    my $opts = sub { pack('C C n C C C C', 2, 4, 1460, 1, 3, 3, $_[0]) };
    push @{$c->{recs}},
      [$t, frame($CLIENT, $SERVER, $CPORT, $SPORT, $c->{cseq} - 1, '', 0x02, 0, 65535, $opts->($cws // 6))],
      [
       $t + 0.0001,
       frame($SERVER, $CLIENT, $SPORT, $CPORT, $c->{sseq} - 1, '', 0x12, $c->{cseq}, 65535, $opts->($sws // 7))
      ],
      [$t + 0.0003, frame($CLIENT, $SERVER, $CPORT, $SPORT, $c->{cseq}, '', 0x10, $c->{sseq})];
    return;
}

sub c2s {
    my ($c, $t, $payload, $window) = @_;
    push @{$c->{recs}},
      [$t, frame($CLIENT, $SERVER, $CPORT, $SPORT, $c->{cseq}, $payload, 0x18, $c->{sseq}, $window // 1024)];
    $c->{cseq} += length $payload;
    return;
}

# payload in segments of mss, the first at t, 0.1 ms apart; the time the last
# went out
sub s2c {
    my ($c, $t, $payload, $mss) = @_;
    $mss //= 1448;
    for (my $i = 0 ; $i < length $payload ; $i += $mss) {
        my $part = substr($payload, $i, $mss);
        push @{$c->{recs}}, [$t, frame($SERVER, $CLIENT, $SPORT, $CPORT, $c->{sseq}, $part, 0x18, $c->{cseq})];
        $c->{sseq} += length $part;
        $t += 0.0001;
    }
    return $t - 0.0001;
}

sub ack {
    my ($c, $t, $window) = @_;
    push @{$c->{recs}},
      [$t, frame($CLIENT, $SERVER, $CPORT, $SPORT, $c->{cseq}, '', 0x10, $c->{sseq}, $window // 1024)];
    return;
}

sub analyse {
    my ($recs, $tmp, $name, $keep) = @_;
    my $path = "$tmp/$name.pcap";
    write_pcap($path, ref $recs eq 'HASH' ? $recs->{recs} : $recs);
    my $tokens = NetatalkDiag::new_names($keep // 1);
    my ($conns, $rows) = NetatalkDiag::analyse_capture($path, 548, $tokens);
    return ($conns, $rows, $tokens, $path);
}

sub csv_line {
    my ($line) = @_;
    my @f;
    while ($line =~ /\G(?:"((?:[^"]|"")*)"|([^,]*))(,|\z)/g) {
        push @f, defined $1 ? $1 =~ s/""/"/gr : $2;
        last if $3 eq '';
    }
    return @f;
}

sub csv_rows {
    my ($rows, $tmp, $name) = @_;
    my $path = "$tmp/$name.csv";
    NetatalkDiag::write_csv($rows, $path);
    my @lines = split /\r\n/, slurp($path);
    my @head  = csv_line(shift @lines);
    return map {
        my %r;
        @r{@head} = csv_line($_);
        \%r
    } @lines;
}

sub test_requests {
    my ($tmp) = @_;
    my ($cseq, $sseq, $t, @recs) = (1000, 9000, 100.0);

    # the capture starts inside a request: half of a frame first
    my $stale = dsi(0, 2, 7, getfiledirparms(2, 'stale'));
    my $half  = int(length($stale) / 2);
    push @recs, [$t, frame($CLIENT, $SERVER, $CPORT, $SPORT, $cseq, substr($stale, $half))];
    $cseq += length($stale) - $half;
    my @reqs = (
                getfiledirparms(2, 'navmark-down'),
                enumext2(17),
                getfiledirparms(17, "Icon\r"),
                getfiledirparms(2,  'navmark-up'),
                getfiledirparms(57, "a\0\0\0a"),
                enumext2(57),
    );
    for my $i (0 .. $#reqs) {
        $t += 0.010;
        my $req = dsi(0, 2, 100 + $i, $reqs[$i]);
        push @recs, [$t, frame($CLIENT, $SERVER, $CPORT, $SPORT, $cseq, $req)];
        $cseq += length $req;
        my $rep = dsi(1, 2, 100 + $i, "\0" x 20, $i == 2 ? -5018 : 0);
        push @recs, [$t + 0.001, frame($SERVER, $CLIENT, $SPORT, $CPORT, $sseq, $rep)];
        $sseq += length $rep;
    }
    my (undef, $rows, undef, $cap) = analyse(\@recs, $tmp, 'requests');
    my @out;
    NetatalkDiag::report_requests($rows, \@out);
    my $text = join("\n", @out);
    my @got  = csv_rows($rows, $tmp, 'requests');
    my @work = grep { index($_->{path}, 'navmark') < 0 } @got;
    ok(has($text, '== phase down:', '== phase up:'),   'two phases from the markers');
    ok(has($text, 'phase down: 2 requests'),           'down holds two requests');
    ok(has($text, 'phase up: 2 requests'),             'up holds two requests');
    ok(!grep({ $_->{server_us} ne '1000' } @got),      'server time is the 1 ms gap');
    ok(!grep({ $_->{client_gap_us} ne '9000' } @work), 'the client gap is the 9 ms between them');
    is_deeply([map { $_->{result} } grep { $_->{path} eq 'Icon\r' } @work], ['-5018'],
              'the error reply keeps its code');
    is_deeply(
              [
               map  { [@{$_}{qw(names hops path)}] }
               grep { $_->{did} eq '57' && $_->{cmd} eq 'FPGetFileDirParms' } @work
              ],
              [['2', '2', 'a/../../a']],
              'two parent hops decoded'
    );
    ok(!grep({ index($_->{path}, 'stale') >= 0 } @got), 'a half frame at the start is skipped');
    is(scalar @got, 6, 'six requests in the CSV');
    ok(!grep({ $_->{conn} ne 'c1' } @got), 'the connection is numbered, not addressed');
    ok($text =~ /slow: .* bitmaps f0001 d0100$/m && !grep({ $_->{bitmaps} ne 'f0001 d0100' } @got),
       'the slow lines and the CSV carry the request bitmaps');

    # the same capture with the names replaced by tokens
    my (undef, $tok_rows, $tokens) = analyse(\@recs, $tmp, 'tokens', 0);
    my $paths = sub {
        my ($did) = @_;
        return map { $_->{path} } grep { ($_->{did} // -1) == $did && $_->{cmd} eq 'FPGetFileDirParms' } @{$tok_rows};
    };
    my @parts = split m{/}, ($paths->(57))[0] // '';
    ok(
          join('|', $paths->(17)) eq 'Icon\r'
       && (grep { index($_->{path}, 'navmark-') >= 0 } @{$tok_rows}) == 2
       && @parts == 4
       && $parts[0] eq $parts[3]
       && $parts[0] =~ /\An[0-9a-f]{10}\z/
       && $tokens->{seen}{a},
       'names are tokens, the markers and Icon\r kept'
    );

    # the same capture cut off five bytes into its last record's frame
    my $whole = slurp($cap);
    my ($pos, $last) = (24, 24);
    while ($pos + 16 <= length $whole) {
        $last = $pos;
        $pos += 16 + unpack('V', substr($whole, $pos + 8, 4));
    }
    spew("$tmp/cut.pcap", substr($whole, 0, $last + 16 + 5));
    my (undef, $cut_rows) = NetatalkDiag::analyse_capture("$tmp/cut.pcap", 548, NetatalkDiag::new_names(1));
    ok(@{$cut_rows} >= 5, 'a capture cut inside a record still parses');
    return;
}

sub test_pipelined_and_lost {
    my ($tmp) = @_;
    my @recs = (
          [
           200.0, frame($CLIENT, $SERVER, $CPORT, $SPORT, 1000, dsi(0, 2, 1, enumext2(17)) . dsi(0, 2, 2, enumext2(18)))
          ]
    );
    my $sseq = 9000;
    for my $r ([1, 200.001], [2, 200.003]) {
        my $rep = dsi(1, 2, $r->[0], "\0" x 20);
        push @recs, [$r->[1], frame($SERVER, $CLIENT, $SPORT, $CPORT, $sseq, $rep)];
        $sseq += length $rep;
    }
    my (undef, $rows) = analyse(\@recs, $tmp, 'piped');
    is_deeply([map { sprintf('%.0f', $_->{server_us}) } @{$rows}],
              ['1000', '2000'], "a queued request's server time starts at the reply before it");
    my @out;
    NetatalkDiag::transfers($rows, \@out);
    ok(scalar(grep { index($_, 'in flight: at most 2') >= 0 } @out), 'two requests were in flight at once');
    my $cseq = 1000;
    ($sseq, @recs) = (9000);
    for my $i (0 .. 4) {
        my $t   = 300.0 + $i * 0.010;
        my $req = dsi(0, 2, 1 + $i, enumext2(1 + $i));
        push @recs, [$t, frame($CLIENT, $SERVER, $CPORT, $SPORT, $cseq, $req)] if $i != 1;
        $cseq += length $req;
        my $rep = dsi(1, 2, 1 + $i, "\0" x 20);
        push @recs, [$t + 0.001, frame($SERVER, $CLIENT, $SPORT, $CPORT, $sseq, $rep)];
        $sseq += length $rep;
    }
    my ($conns, $gap_rows) = analyse(\@recs, $tmp, 'gap');
    ok(join(',', sort { $a <=> $b } map { $_->{did} } @{$gap_rows}) eq '1,3,4,5' && grep({ $_->{c2s}{gaps} } @{$conns}),
       'requests after a lost client segment are parsed');

    # 100 lost bytes, then a reply whose own bytes, read one byte early, would
    # complete a header with the filler's last byte as its flags
    my $st = NetatalkDiag::new_stream();
    NetatalkDiag::stream_add($st, 999,  0x02, '',                       1.0);
    NetatalkDiag::stream_add($st, 1100, 0x18, dsi(1, 2, 9, "\0" x 256), 1.1);
    NetatalkDiag::stream_finish($st);
    ok(!NetatalkDiag::frame_at(\$st->{data}, 99), 'no frame starts in the filler of lost bytes');
    return;
}

# a login, an icon's resource fork read in two sequential reads, a
# retransmission, a zero window, a write and two pipelined requests
sub session_capture {
    my $c = capture();
    handshake($c, 100.0);
    c2s($c, 100.001, dsi(0, 4, 1, "\x01\x04\x00\x00\x04\x00"));
    s2c($c, 100.002, dsi(1, 4, 1, "\x00\x04\x00\x10\x00\x00"));
    c2s($c, 100.003, dsi(0, 2, 2, pack('C C', 63, 0) . 'uam'));
    s2c($c, 100.010, dsi(1, 2, 2, "\0" x 8, -5001));
    c2s($c, 100.011, dsi(0, 2, 3, pack('C C', 19, 0) . 'cont'));
    s2c($c, 100.020, dsi(1, 2, 3, ''));
    c2s($c, 100.021, dsi(0, 2, 4, pack('C C n', 24, 0, 0) . "\x01v"));
    s2c($c, 100.022, dsi(1, 2, 4, "\0" x 4));
    c2s($c, 100.030, dsi(0, 2, 5, openfork(17, "Icon\r", 1)));
    s2c($c, 100.031, dsi(1, 2, 5, pack('n n', 0x0200, 7) . ("\0" x 4)));
    c2s($c, 100.032, dsi(0, 2, 6, readext(7, 0, 65536)));
    my $end = s2c($c, 100.033, dsi(1, 2, 6, "\x11" x 65536));
    ack($c, $end + 0.0005);
    c2s($c, 100.050, dsi(0, 2, 7, readext(7, 65536, 65536)));
    my $first = $c->{sseq};
    $end = s2c($c, 100.051, dsi(1, 2, 7, "\x22" x 65536));

    # the server sends its first segment of that reply again, and the client
    # advertises a zero window before it catches up
    push @{$c->{recs}}, [$end + 0.0001, frame($SERVER, $CLIENT, $SPORT, $CPORT, $first, "\0" x 1448, 0x18, $c->{cseq})];
    ack($c, $end + 0.0002, 0);
    ack($c, $end + 0.0010);
    c2s($c, 100.070, dsi(0, 6, 8, pack('C C n', 61, 0, 7) . be64(0) . be64(4096) . ("\x33" x 4096), 20));
    s2c($c, 100.071, dsi(1, 2, 8, "\0" x 8));
    c2s($c, 100.080, dsi(0, 2, 9, getfiledirparms(2, 'x')) . dsi(0, 2, 10, getfiledirparms(2, 'y')));
    s2c($c, 100.081, dsi(1, 2, 9,  "\0" x 20));
    s2c($c, 100.082, dsi(1, 2, 10, "\0" x 20));
    return $c;
}

sub test_session {
    my ($tmp) = @_;
    my ($conns, $rows) = analyse(session_capture(), $tmp, 'session');
    my ($s,     $c)    = @{$conns->[0]}{qw(server client)};
    ok($s->{retrans} == 1 && $c->{retrans} == 0, "the server's resent segment counts as a retransmission");
    ok($c->{zero_windows} == 1,                  "the client's zero window counts");
    ok(@{$s->{rtts}} && min(@{$s->{rtts}}) > 0 && min(@{$s->{rtts}}) < 0.01, 'server data gets round-trip samples');
    is_deeply([$c->{mss}, $s->{mss}, $c->{wscale}, $s->{wscale}], [1460, 1460, 6, 7],
              'the SYNs give MSS and window scale');
    my @out;
    NetatalkDiag::tcp_health($conns, \@out);
    ok(has(join("\n", @out), 'handshake round trip: 0.20 ms', "filled the client's window"),
       'the TCP report shows the handshake and the window figures');
    @out = ();
    NetatalkDiag::logins($conns, $rows, \@out);
    ok(
       has(
           join("\n", @out), 'session open 2.0 ms', 'login 17.0 ms in 2 requests, result 0', 'first volume open 22.0 ms'
       ),
       'the login report times the session, the login and the volume'
    );
    @out = ();
    NetatalkDiag::transfers($rows, \@out);
    my $text = join("\n", @out);
    ok(
       has(
           $text, 'reads: 2 on 1 fork, 0.12 MiB', '<256K 2', 'sequential 1 of 1 later reads on a fork', 'forks rsrc 2'
       ),
       'the reads are counted with their size and fork'
    );
    ok(has($text, 'writes: 1 on 1 fork, 0.00 MiB', '<64K 1'), 'the write counts its data, not its AFP header');
    my @got = csv_rows($rows, $tmp, 'session');
    is_deeply(
              [map { [@{$_}{qw(reply_bytes fork offset)}] } grep { $_->{cmd} eq 'FPReadExt' } @got],
              [['65536', 'rsrc', '0'], ['65536', 'rsrc', '65536']],
              "the CSV carries each read's size, fork and offset"
    );
    is_deeply(
              [map { $_->{cmd} } @got[0 .. 3]],
              ['DSIOpenSession', 'FPLoginExt', 'FPLoginCont', 'FPOpenVol'],
              'DSIOpenSession is a request of its own'
    );

    # an offset past 4 GiB, which a 32-bit perl must read too
    my $big = capture();
    c2s($big, 600.0, dsi(0, 2, 1, readext(9, 8589934592 + 4096, 65536)));
    s2c($big, 600.001, dsi(1, 2, 1, "\0" x 16));
    my (undef, $big_rows) = analyse($big, $tmp, 'big');
    is(($big_rows->[0] // {})->{offset}, 8589934592 + 4096, 'a 64-bit read offset reads whole');
    return;
}

sub test_verdicts {
    my ($tmp) = @_;
    my $capture = sub {
        my ($server_ms, $client_ms) = @_;
        my ($c,         $t)         = (capture(), 400.0);
        for my $i (1 .. 10) {
            c2s($c, $t, dsi(0, 2, $i, enumext2(17)));
            $t = s2c($c, $t + $server_ms / 1000, dsi(1, 2, $i, "\0" x 20)) + $client_ms / 1000;
        }
        return $c;
    };
    for my $case (['slow server', 50, 1, 'afpd set the pace'], ['slow client', 1, 50, 'the client set the pace']) {
        my ($name, $server, $client, $want) = @{$case};
        my (undef, $rows) = analyse($capture->($server, $client), $tmp, $name);
        my @out;
        NetatalkDiag::verdicts($rows, \@out);
        ok(scalar(grep { index($_, $want) >= 0 } @out), "the verdict names the side that took the time: $name");
    }
    return;
}

my $CONF = <<'EOF';
[Global]
hostname = myserver.example.org
log file = /var/log/afpd.log
spotlight backend = cnid
log level = default:debug
cnid scheme = mysql
cnid mysql host = db.example.org
tcpsndbuf = 65536
dircache size = 4096
dircache validation freq = 1
use sendfile = no
server quantum = 262144
dircache rfork budget = 0
; a comment
[Family Photos]
path = /srv/Family Photos
valid users = alice
multi protocol = yes
[Homes]
basedir regex = /home
EOF

sub test_conf {
    my $sections = NetatalkDiag::parse_conf($CONF);
    my $red      = join("\n", NetatalkDiag::redact_conf($sections));
    ok(
       has(
           $red, '[volume 1]', '[Homes]', ' hostname = REDACTED', ' cnid mysql host = REDACTED', ' path = REDACTED',
           ' valid users = REDACTED', ' log level = default:debug', ' spotlight backend = cnid', ' log file = REDACTED',
           ' dircache size = 4096'
       )
       && $red !~ /Family|myserver|comment/,
       'the configuration keeps its option names, its shaping values and no identifying ones'
    );
    my @notes;
    NetatalkDiag::config_notes($sections, \@notes);
    ok(
       has(
           join("\n", @notes),
           'debug logging',       'cnid scheme = mysql',
           'tcpsndbuf = 65536',   'dircache size = 4096',
           'validation freq = 1', 'multi protocol = yes',
           'use sendfile = no',   'server quantum = 262144',
           'rfork budget = 0'
       ),
       'each costly setting gets its note'
    );
    @notes = ();
    NetatalkDiag::config_notes(
                               NetatalkDiag::parse_conf(
                                                        "[Global]\nlog level = default:note\ndircache size = 65536\n"),
                               \@notes
    );
    is_deeply(\@notes, [], 'default settings get no note');
    is_deeply([NetatalkDiag::volume_paths($sections)], [[1, '/srv/Family Photos']],
              'volume paths come from the volume sections');
    return;
}

my $MOUNTINFO = <<'EOF';
22 1 8:1 / / rw,relatime shared:1 - ext4 /dev/sda1 rw
36 22 0:40 / /srv rw,noatime shared:5 - nfs4 fileserver:/export/srv rw,vers=4.2,rsize=1048576,addr=10.9.9.9
EOF
my $MOUNT_P = "zroot/ROOT/default\t/\tzfs\trw\t0\t0\ntank/media\t/tank/media\tzfs\trw,nfsv4acls\t0\t0\n";

sub test_mounts {
    my $linux = NetatalkDiag::read_mounts($MOUNTINFO);
    ok(
       scalar(
              grep { "@{$_}" eq '/srv nfs4 fileserver:/export/srv rw,noatime,rw,vers=4.2,rsize=1048576,addr=10.9.9.9' }
              @{$linux}
       ),
       'Linux mountinfo reads mount point, type, source and both option sets'
    );
    my $bsd = NetatalkDiag::read_mounts($MOUNT_P);
    ok(
       scalar(grep { "@{$_}" eq '/tank/media zfs tank/media rw,nfsv4acls' } @{$bsd}),
       'FreeBSD mount -p reads mount point, type and source'
    );
    ok(
          NetatalkDiag::mount_of('/tank/media/photos', $bsd)->[0] eq '/tank/media'
       && NetatalkDiag::mount_of('/usr', $bsd)->[0] eq '/',
       'a path takes the mount that holds it'
    );
    my @lines = NetatalkDiag::volume_lines(NetatalkDiag::parse_conf($CONF), $linux);
    ok(@lines == 1 && has($lines[0], 'nfs4', 'network filesystem', 'rsize=1048576') && index($lines[0], '10.9.9.9') < 0,
       'a network filesystem is flagged and its options lose their addresses');
    return;
}

sub test_redaction {
    is(NetatalkDiag::depth('open /srv/a b/c.txt here'), 'open /+/+/+', 'paths keep their depth');
    is(
       NetatalkDiag::addr('from 10.1.2.3:548 [fe80::1]:548 ether 00:11:22:33:44:55'),
       'from ADDR ADDR ether MAC', 'addresses and hardware addresses go'
    );
    is(NetatalkDiag::ghosts('12: 3 4 5 d   secret name (GHOST)'), '12: 3 4 5 d   + (GHOST)',
       'a ghost entry loses its name');
    my $names = [['tank/media', 'DATASET1'], ['tank', 'POOL1']];
    my $zfs = NetatalkDiag::zfs_redact(
                                         "NAME PROPERTY VALUE SOURCE\n"
                                       . "tank/media compression lz4 local\n"
                                       . "tank/media guid 12345 -\n"
                                       . "tank/media mountpoint /tank/media default\n"
                                       . "tank/media org.acme:owner alice local\n"
                                       . "tank/media atime off inherited from tank\n",
                                       $names
    );
    ok($zfs !~ /tank|12345|alice/ && has($zfs, 'DATASET1 compression lz4', 'DATASET1 guid REDACTED -'),
       'zfs get loses names, the guid, the mount point and user properties');
    my $pool = NetatalkDiag::zpool_redact(
                                            "  pool: tank\n state: ONLINE\nconfig:\n\n"
                                          . "\tNAME        STATE\n\ttank        ONLINE\n"
                                          . "\t  mirror-0  ONLINE\n"
                                          . "\t    ata-WDC_SERIAL123  ONLINE\n"
                                          . "\t    gpt/disk1  ONLINE\n\n"
                                          . "errors: Permanent errors\n\n"
                                          . "        /tank/media/secret.jpg\n",
                                          $names
    );
    ok($pool !~ m{SERIAL123|gpt/disk1|secret} && has($pool, 'POOL1', 'mirror-0', 'ENTRY'),
       "zpool status loses devices, the pool's name and error paths");
    return;
}

sub test_leak_check {
    my ($tmp) = @_;
    my $s = NetatalkDiag::new_sensitive();
    NetatalkDiag::sens_add($s, $_) for 'alice', 'Family Photos', 'data', 'ab', 'SecretFolder', 'IMG_1234.JPG';
    NetatalkDiag::sens_add_literal($s, '10.0.0.2');
    ok(!$s->{words}{data} && !$s->{words}{ab}, 'generic words and short strings are not watched');
    my $allowed = NetatalkDiag::new_sensitive();
    NetatalkDiag::sens_allow($allowed, 'vetoed');
    NetatalkDiag::sens_add($allowed, 'vetoed');
    NetatalkDiag::sens_add($s,       'vetoed');
    ok(!$allowed->{words}{vetoed} && $s->{words}{vetoed}, 'an allowed word, as an option name is, is not watched');
    my $scan = sub { [NetatalkDiag::sens_scan($s, $_[0])] };
    is_deeply([$scan->('user alice here'), $scan->('malice')], [['alice'], []],
              'a watched word is found as a whole word only');
    is_deeply([$scan->('path Family Photos/x'), $scan->('a IMG_1234.JPG b')],
              [['Family Photos'], ['IMG_1234.JPG']], 'a watched phrase is found');
    is_deeply([$scan->('peer 10.0.0.2:548'), $scan->('peer 10.0.0.23')],
              [['10.0.0.2'], []], 'an address is found with a port and not inside another one');

    # a folder name in another script is a word too
    my $photos = "\x{424}\x{43e}\x{442}\x{43e}";
    NetatalkDiag::sens_add($s, $photos);
    is_deeply($scan->("x $photos y"), [$photos], 'a name in any script is watched');
    my $share = "$tmp/leakshare";
    mkdir $share or die "$share: $!";
    spew("$share/report.txt",         "one\nSecretFolder two\n");
    spew("$share/counters-after.txt", "kstat.zfs.misc.SecretFolder: 1\n");
    is_deeply([NetatalkDiag::leak_scan($share, $s)], [['report.txt', 2, 'SecretFolder']],
              'the scan reports the file and line, and skips counter files');
    return;
}

sub outdir_with {
    my ($tmp, $name, $extra) = @_;
    my $out = "$tmp/$name";
    mkdir $out or die "$out: $!";
    my $c = capture();
    c2s($c, 500.0, dsi(0, 2, 1, getfiledirparms(2, 'SecretFolder')));
    s2c($c, 500.001, dsi(1, 2, 1, "\0" x 20));
    c2s($c, 500.010, dsi(0, 2, 2, getfiledirparms(2, "SecretFolder\0Icon\r")));
    s2c($c, 500.011, dsi(1, 2, 2, "\0" x 20));
    write_pcap("$out/afp.pcap", $c->{recs});
    my %files = (
        'afp.conf.raw' => $CONF,
        'identity.txt' =>
          "host myserver.example.org\naddress 10.0.0.1\nclient 10.0.0.2\nuser alice\nmac 00:11:22:33:44:55\n",
        'sessions.txt'    => "1234 alice 10.0.0.2 1\n",
        'mounts.txt'      => $MOUNTINFO,
        'system.txt'      => "Linux 6.1.0 x86_64\n",
        'network.txt'     => "eth0 mtu 1500 inet 10.0.0.1/24 ether 00:11:22:33:44:55\n",
        'ps-before.txt'   => "PID PPID USER %CPU\n1234 1 alice 0.0\n",
        'times.txt'       => "100 110\n",
        'proc-before.txt' =>
          "== snmp\nTcp: RetransSegs OutSegs\nTcp: 5 100\n== pid.stat\n1234 (afpd) S 1 1 1 0 -1 0 0 0 0 0 10 5 0 0\n",
        'proc-after.txt' =>
          "== snmp\nTcp: RetransSegs OutSegs\nTcp: 9 300\n== pid.stat\n1234 (afpd) S 1 1 1 0 -1 0 0 0 0 0 110 25 0 0\n",
        'notes.txt' => "no tracing: -t was not given\n",
        %{$extra // {}},
    );
    spew("$out/$_", $files{$_}) for keys %files;
    return $out;
}

sub run_report {
    my ($out, $force) = @_;
    my $err = '';
    open(my $saved, '>&', \*STDERR) or die "dup: $!";
    close STDERR;
    open(STDERR, '>', \$err) or die "stderr: $!";
    my $status = eval { NetatalkDiag::report($out, "$out/reports", $force) };
    my $died   = $@;
    close STDERR;
    open(STDERR, '>&', $saved) or die "restore: $!";
    return (defined $status ? $status : "died: $died", $err);
}

sub test_report {
    my ($tmp) = @_;
    my $out = outdir_with($tmp, 'out1');
    my ($status, $err) = run_report($out);
    my %texts;
    opendir(my $dh, "$out/reports") or die "$out/reports: $!";
    $texts{$_} = slurp("$out/reports/$_") for grep { !/\A\./ } readdir $dh;
    closedir $dh;
    my $every = join("\n", values %texts);
    ok($status eq '0' && $err eq '', 'the report finds no leak in what it wrote') or diag("status $status: $err");
    ok(
       !grep({ index($every, $_) >= 0 } 'SecretFolder', 'Family Photos', 'myserver', 'alice', '10.0.0.2', '10.0.0.1',
             '00:11:22:33:44:55', 'db.example.org', 'fileserver', 'export'),
       'no shared file names a folder, the volume, the host, a user or an address'
    );
    for my $name (
                  qw(summary.txt report-pcap.txt requests.csv afp.conf sessions.txt volumes.txt network.txt README.txt
                  counters-before.txt ps-before.txt report-counters.txt)
    ) {
        ok(exists $texts{$name}, "the reports hold $name");
    }
    ($status) = run_report($out);
    ok(index($status, 'died: ') == 0 && slurp("$out/reports/summary.txt") eq ($texts{'summary.txt'} // ''),
       'a report into a folder that exists stops and leaves its files');
    ok(has($texts{'summary.txt'} // '', 'verdict:', 'cnid scheme = mysql', 'no tracing: -t was not given'),
       'the summary has a verdict, configuration and collection notes');

    # 120 ticks of /proc's clock, which the report reads from the system
    my $cpu = sprintf('%.2f s on CPU', 120 / POSIX::sysconf(POSIX::_SC_CLK_TCK()));
    ok(has($texts{'report-counters.txt'} // '', 'Tcp:RetransSegs', $cpu),
       "the counter report shows the TCP change and afpd's CPU");
    ok(
          has($texts{'sessions.txt'}  // '', 'user1, client1, traced')
       && has($texts{'ps-before.txt'} // '', '1234 1 user1'),
       'sessions and processes are numbered'
    );
    ok(($texts{'requests.csv'} // '') =~ m{n[0-9a-f]{10}/Icon\\r},
       'the request report keeps Icon\r and tokens the folder');
    my $leak = {'tcpdump.txt' => "listening on myserver eth0\n"};
    ($status, $err) = run_report(outdir_with($tmp, 'out2', $leak));
    ok($status eq '3' && has($err, "tcpdump.txt line 1 holds 'myserver'"), 'a leak stops the report');
    ($status) = run_report(outdir_with($tmp, 'out3', $leak), 1);
    ok($status eq '0', '--force packages after a reported leak');

    # a user who logs in during the run shows in the after snapshot alone
    my $late = {
                'ps-after.txt' => "PID PPID USER %CPU\n4321 1 latecomer 0.0\n",
                'tcpdump.txt'  => "listening on eth0 for latecomer\n",
    };
    ($status, $err) = run_report(outdir_with($tmp, 'out4', $late));
    ok($status eq '3' && has($err, "tcpdump.txt line 1 holds 'latecomer'"),
       'a user who appears only in the after snapshot is watched too');
    return;
}

sub test_counters {
    my $linux =
      NetatalkDiag::load_linux(  "== snmp\nTcp: RtoAlgorithm RetransSegs\nTcp: 1 7\n"
                               . "== netstat\nTcpExt: TCPTimeouts TCPToZeroWindowAdv\nTcpExt: 2 3\n"
                               . "== vmstat\npgmajfault 11\n"
                               . "== diskstats\n   8       0 sda 10 0 80 5 20 0 160 15 0 30 40\n"
                               . "== arcstats\n13 1 0x01 1 1 1 1\nname type data\nhits 4 99\n"
                               . "== pressure.io\nsome avg10=0.00 avg60=0.00 avg300=0.00 total=555\n"
                               . "== pid.status\nvoluntary_ctxt_switches:\t12\nVmRSS:\t  2048 kB\n"
                               . "== pid.io\nread_bytes: 4096\n"
                               . "== pid.fds\n17\n"
                               . "== if\nrx_bytes: 1000\n");
    my %want = (
                'Tcp:RetransSegs'                    => 7,
                'TcpExt:TCPToZeroWindowAdv'          => 3,
                'vmstat.pgmajfault'                  => 11,
                'disk.sda.writes'                    => 20,
                'disk.sda.io_ms'                     => 30,
                'kstat.zfs.misc.arcstats.hits'       => 99,
                'pressure.io.some.total_us'          => 555,
                'pid.status.voluntary_ctxt_switches' => 12,
                'pid.status.VmRSS'                   => 2048,
                'pid.io.read_bytes'                  => 4096,
                'pid.fds'                            => 17,
                'if.rx_bytes'                        => 1000,
    );
    ok(!grep({ ($linux->{$_} // -1) != $want{$_} } keys %want), 'Linux counters parse from their proc formats');
    my %after =
      (%{$linux}, 'disk.sda.reads' => 20, 'disk.sda.read_ms' => 25, 'disk.sda.io_ms' => 530, 'Tcp:RetransSegs' => 9);
    my @out;
    NetatalkDiag::counter_report($linux, \%after, 10.0, \@out);
    ok(has(join("\n", @out), 'disk sda: 10 I/Os', '2.00 ms each', 'busy 5%', 'Tcp:RetransSegs'),
       "the counter report gives each disk's I/Os, latency and busy time");
    my %before = ('pid.utime_ticks' => 100, 'pid.stime_ticks' => 50);
    for my $disk (qw(vda vda1 nvme0n1 nvme0n1p2)) {
        $before{"disk.$disk.$_"}    = 0 for qw(writes read_ms write_ms read_sectors write_sectors io_ms);
        $before{"disk.$disk.reads"} = 1;
    }
    %after = %before;
    $after{"disk.$_.reads"} = 5 for qw(vda vda1 nvme0n1 nvme0n1p2);
    delete @after{qw(pid.utime_ticks pid.stime_ticks)};
    @out = ();
    NetatalkDiag::counter_report(\%before, \%after, 10.0, \@out);
    my $text = join("\n", @out);
    ok(has($text, 'disk vda:', 'disk nvme0n1:') && $text !~ /vda1|nvme0n1p2/,
       "a partition's I/O is not counted again beside its disk's");
    ok(index($text, 'on CPU') < 0, "a process that ended before the run's end gets no CPU line");
    my $bsd = NetatalkDiag::load_netstat_s(
                "tcp:\n\t1234 packets sent\n\t\t12 data packets (1234 bytes) retransmitted\n\t5 retransmit timeouts\n");
    ok(
          ($bsd->{'tcp N data packets (N bytes) retransmitted'} // 0) == 12
       && ($bsd->{'tcp N retransmit timeouts'} // 0) == 5,
       'FreeBSD netstat -s lines parse by their wording'
    );
    my $table = NetatalkDiag::load_table(
                                           "Name Mtu Network Address Ipkts Ierrs Idrop Ibytes Opkts Oerrs Obytes Coll\n"
                                         . "mce0 9000 10.0.0.0/24 10.0.0.1 9 0 0 9 9 0 9 0\n"
                                         . "mce0 9000 <Link#1> 00:11:22:33:44:55 100 1 2 5000 200 3 9000 0\n",
                                         'mce0'
    );
    is_deeply(
              $table,
              {
               'if.Ipkts'  => 100,
               'if.Ierrs'  => 1,
               'if.Idrop'  => 2,
               'if.Ibytes' => 5000,
               'if.Opkts'  => 200,
               'if.Oerrs'  => 3,
               'if.Obytes' => 9000,
               'if.Coll'   => 0
              },
              "FreeBSD netstat -I keeps the link-level row's numbers"
    );
    my $rusage =
      NetatalkDiag::load_rusage(  "  PID COMM             RESOURCE                          VALUE\n"
                                . " 1234 afpd             user time                    00:00:01.500000\n"
                                . " 1234 afpd             maximum RSS                         12345 KB\n"
                                . " 1234 afpd             voluntary context switches           77\n");
    ok(
          ($rusage->{'rusage.user time s'} // 0) == 1.5
       && ($rusage->{'rusage.maximum RSS'}                // 0) == 12345
       && ($rusage->{'rusage.voluntary context switches'} // 0) == 77,
       'FreeBSD procstat -r reads times and counts'
    );
    my $head = "                        extended device statistics\n"
      . "device       r/s     w/s     kr/s     kw/s  ms/r  ms/w  ms/o  ms/t qlen  %b\n";
    @out = ();
    NetatalkDiag::iostat_report(
       "${head}ada0 1 1 1 1 1 1 1 1 0 1\n${head}ada0 10 20 1 1 1 1 1 4.0 0 30\n${head}ada0 30 40 1 1 1 1 1 8.0 0 50\n",
       \@out
    );
    ok(scalar(grep { has($_, 'ada0', 'busy mean 40% peak 50%', 'r/s 20 w/s 30', 'mean 6.0 peak 8.0') } @out),
       'FreeBSD iostat skips its since-boot block and averages the rest');
    return;
}

sub test_traces {
    my @out;
    NetatalkDiag::trace_totals(
                                 "REQ 1000 AFP_READ_EXT 300000 100000 150000 50000\n"
                               . "REQ 2000 AFP_ENUMERATE_EXT2 200000 200000 0 0\n"
                               . "OFF AFP_READ_EXT - select 3 150000000 50000000\n"
                               . "CNID AFP_ENUMERATE_EXT2 cnid_add 2 40000\n",
                               \@out, 'DTrace'
    );
    ok(
       has(
           join("\n", @out),
           '2 requests, 0.500 s in their AFP handlers',
           'on CPU 0.300 s, asleep 0.150 s, runnable 0.050 s',
           'asleep on select: 0.150 s',
           'CNID calls: 2'
       ),
       'the DTrace totals add up its REQ, OFF and CNID lines'
    );
    @out = ();
    NetatalkDiag::trace_totals(
                                 "REQ 1000 AFP_READ_EXT 3000000 1000000 1500000 500000\n"
                               . "\@offs[AFP_READ_EXT, S]: 1500000000\n"
                               . "\@cnid[AFP_READ_EXT, cnid_get]: 4\n"
                               . "\@cnidns[AFP_READ_EXT, cnid_get]: 8000\n",
                               \@out, 'bpftrace'
    );
    ok(has(join("\n", @out), 'asleep on S: 1.500 s', 'CNID calls: 4', 'on CPU 1.000 s'),
       'the bpftrace totals read its maps');
    @out = ();
    NetatalkDiag::trace_totals(
                                 "REQ 1000 AFP_READ_EXT 1 1 0 0\nLost 5 events\n"
                               . "\@n[AFP_READ_EXT]: 7\n\@ns[AFP_READ_EXT]: 2000000000\n"
                               . "\@oncpu[AFP_READ_EXT]: 500000000\n\@asleep[AFP_READ_EXT]: 1000000000\n"
                               . "\@runnable[AFP_READ_EXT]: 500000000\n",
                               \@out, 'bpftrace'
    );
    ok(
       has(
           join("\n", @out),
           '7 requests, 2.000 s in their AFP handlers',
           'on CPU 0.500 s, asleep 1.000 s, runnable 0.500 s',
           'lost 5 events'
       ),
       'the end-of-trace counts win over REQ lines a full buffer lost'
    );
    @out = ();
    NetatalkDiag::syscall_totals(
                                   "SYS openat 10 3000000000\nSYS read 90 1000000000\n"
                                 . "\@n[tracepoint:syscalls:sys_exit_read]: 10\n"
                                 . "\@ns[tracepoint:syscalls:sys_exit_read]: 500000000\n",
                                 \@out, 'trace'
    );
    my $text = join("\n", @out);
    ok(
          has($text, '110 calls, 4.500 s')
       && $text =~ /openat +10 calls +3\.000 s/
       && $text =~ /read +100 calls +1\.500 s/,
       "the system call totals add up DTrace's lines and bpftrace's maps"
    );
    @out = ();
    NetatalkDiag::samples_report("hz 100\nSAMPLE S do_select 80\nSAMPLE D io_schedule 15\nSAMPLE R - 5\n", \@out);
    ok(has(join("\n", @out), 'asleep', '80.0%', 'D on io_schedule', '15.0%'),
       "the samples give each state's share and the wait channels");
    return;
}

sub test_log {
    my ($tmp) = @_;
    spew(
          "$tmp/afpd.1234.abc",
          "Nov 10 03:11:00.000000 afpd[1] {x} <== Start AFP command: AFP_GETFLDRPARAM\n"
          . "Nov 10 03:11:00.000100 afpd[1] {x} dir {not in cache}\n"
          . "Nov 10 03:11:00.000500 afpd[1] {x} ==> Finished AFP command: AFP_GETFLDRPARAM -> AFP_OK\n"
          . "Nov 10 03:11:01.000000 afpd[1] {x} dircache statistics (ARC): (user: alice) entries: 0, lookups: 9\n"
    );
    my @out;
    NetatalkDiag::log_report(["$tmp/afpd.1234.abc"], \@out);
    my $text = join("\n", @out);
    ok(has($text, '1 requests, 0.5 ms in afpd', 'miss 1'), 'the maxdebug log report times requests and counts events');
    ok(has($text, 'dircache statistics (ARC): entries: 0, lookups: 9') && index($text, 'alice') < 0,
       'the session statistics leave out the user afpd names in them');
    return;
}

sub test_baseline {
    my ($tmp) = @_;
    my $root = "$tmp/vol";
    mkdir $_ or die "$_: $!" for $root, "$root/a", "$root/a/b", "$root/c";
    spew("$root/$_", '') for qw(a/1 a/b/2 c/3 4);
    spew("$tmp/base.conf", "[Global]\n[vol]\npath = $root\n");
    my @lines = NetatalkDiag::baseline("$tmp/base.conf", 100);
    ok(
          @lines
       && $lines[0] =~ /\Avolume 1: 7 entries/
       && grep({ /lstat/ && /n      7/ } @lines)
       && grep({/list a folder/} @lines),
       'the baseline walks every entry and times each step'
    );
    @lines = NetatalkDiag::baseline("$tmp/base.conf", 2);
    like($lines[0] // '', qr/\Avolume 1: 2 entries/, 'the baseline stops at its limit');
    ok(!grep({ index($_, $root) >= 0 } NetatalkDiag::baseline("$tmp/base.conf", 100)),
       'the baseline output names no path');
    return;
}

sub test_programs {
    my $bt = NetatalkDiag::program('afpd_requests.bt', 4242, '/opt/sbin/afpd');
    ok(has($bt, 'usdt:/opt/sbin/afpd:afp:afpfunc__start', 'pid == 4242') && $bt !~ /PID|AFPD/,
       'the bpftrace program gets the pid and the binary');

    # D joins no adjacent string literals: a literal ending one line and
    # another opening the next is a syntax error there
    my %programs = do { no warnings 'once'; %NetatalkDiag::PROGRAMS };
    ok(!grep({ /\.d\z/ && $programs{$_} =~ /"\s*\n\s*"/ } keys %programs),
       'no D program relies on adjacent string literals');
    is_deeply(
              [sort keys %programs],
              [qw(afpd_requests.bt afpd_requests.d afpd_syscalls.bt afpd_syscalls.d zfs_lookups.d)],
              'every program is there'
    );
    return;
}

# the -m files: only a regular file of the session's user that no other name
# links to, never through a link, and only below a folder of the session's
# own; afpd's /tmp is a folder of the test's own, so the test writes nothing
# where another user can place a link
sub test_session_files {
    my ($tmp) = @_;
    no warnings 'once';
    my ($pid, $uid, $dir) = (4242, $<, "$tmp/tmpdir");
    my $own = "$dir/netatalk-$pid";
    mkdir $dir or die "$dir: $!";
    local $NetatalkDiag::TMP = $dir;
    my $find = sub {
        [map {s{\A.*/}{}r} NetatalkDiag::session_files($pid, $uid, @_)]
    };
    spew("$dir/dircache.$pid", 'dump');
    spew("$tmp/target",        'private');
    symlink("$tmp/target", "$dir/afpd.$pid.link") or die "symlink: $!";
    POSIX::mkfifo("$dir/afpd.$pid.fifo", 0600)    or die "mkfifo: $!";
    is_deeply($find->('dumps'), ["dircache.$pid"], "the session's own dump is found");
    is_deeply([NetatalkDiag::session_files($pid, $uid + 1, 'dumps')], [],
              "a file of another user is not the session's");
    local $SIG{ALRM} = sub { die "a named pipe blocked the open\n" };
    alarm 10;
    is_deeply($find->('logs'), [], 'a symbolic link and a named pipe are never read');
    alarm 0;
    link("$tmp/target", "$dir/afpd.$pid.hard") or die "link: $!";
    is_deeply($find->('logs'), [], 'a file that another name links to is not read');
    my $marker = "$tmp/marker";
    spew($marker, '');
    my $now = time;
    utime($now,      $now,      $marker)              or die "utime: $!";
    utime($now - 10, $now - 10, "$dir/dircache.$pid") or die "utime: $!";
    my @since = ('dumps', $marker);
    is_deeply($find->(@since), [], 'a dump from before the marker, as an earlier run leaves, does not count');
    utime($now, $now, "$dir/dircache.$pid") or die "utime: $!";
    is_deeply($find->(@since), ["dircache.$pid"], "a dump changed in the marker's own clock tick counts");
    utime($now + 10, $now + 10, "$dir/dircache.$pid") or die "utime: $!";
    is_deeply($find->(@since), ["dircache.$pid"], 'a dump changed after the marker counts');
    mkdir "$tmp/copy" or die "mkdir: $!";
    NetatalkDiag::session_files($pid, $uid, 'dumps', undef, "$tmp/copy", 'first');
    is(slurp("$tmp/copy/dircache.$pid.first"), 'dump', 'the copy holds what the session wrote');
    mkdir $own or die "mkdir: $!";
    spew("$own/dircache.$pid", 'own');
    chmod 0700, $own;
    is_deeply($find->('dumps'), ["dircache.$pid", "dircache.$pid"], "the session's own folder is searched too");
    chmod 0777, $own;
    is_deeply($find->('dumps'), ["dircache.$pid"], 'a folder others can write to is not');
    return;
}

# the sampler on pid until 0.3 s after its file shows it ready: its exit
# status, the file then, and the file at its end
sub run_sampler {
    my ($target, $out) = @_;
    my $pid = fork() // BAIL_OUT("fork: $!");
    if (!$pid) {
        my $ok = eval { NetatalkDiag::sample($target, $out, 200); 1 };
        POSIX::_exit($ok ? 0 : 1);
    }

    # the file appears once the sampler's handlers are in place
    my $waited = 0;
    while (!-s $out && $waited++ < 1000) {
        Time::HiRes::sleep(0.01);
    }
    my $start = slurp($out);
    Time::HiRes::sleep(0.3);
    kill 'TERM', $pid;
    waitpid($pid, 0);
    return ($?, $start, slurp($out));
}

sub test_sampler {
    my ($tmp) = @_;

    # a procfs of two threads, the second without wchan, as NetBSD's is
    my $task = "$tmp/proc/4242/task";
    for my $dir ("$tmp/proc", "$tmp/proc/4242", $task, "$task/1", "$task/2") {
        mkdir $dir or die "$dir: $!";
    }
    spew("$task/1/stat",  "1 (afpd) S 1 1 1 0 -1 0\n");
    spew("$task/1/wchan", 'do_select');
    spew("$task/2/stat",  "2 (afpd) D 1 1 1 0 -1 0\n");
    {
        no warnings 'once';
        local $NetatalkDiag::PROC = "$tmp/proc";
        my ($status, $start, $end) = run_sampler(4242, "$tmp/tree-samples.txt");
        is($start,  "hz 200\n", 'the sampler marks its file before it samples');
        is($status, 0,          'the sampler ends cleanly on SIGTERM');
        ok($end =~ /^SAMPLE S do_select \d+$/m && $end =~ /^SAMPLE D - \d+$/m,
           "the sampler counts each thread's state, a thread without wchan too");
    }

    # this process in the system's procfs: samples where it has the Linux
    # layout, as on Linux and NetBSD, and the header alone where it has not
    my $linux_layout = -d "/proc/$$/task";
    my ($status, undef, $end) = run_sampler($$, "$tmp/samples.txt");
    ok(
       $status == 0 && ($linux_layout ? $end =~ /\Ahz 200\nSAMPLE / : $end eq "hz 200\n"),
       $linux_layout
       ? "the sampler counts this process's thread states"
       : 'the sampler counts nothing where procfs has no task folders'
    );
    return;
}

my $tmp = File::Temp::tempdir(CLEANUP => 1);
test_requests($tmp);
test_pipelined_and_lost($tmp);
test_session($tmp);
test_verdicts($tmp);
test_conf();
test_mounts();
test_redaction();
test_leak_check($tmp);
test_report($tmp);
test_counters();
test_traces();
test_log($tmp);
test_baseline($tmp);
test_programs();
test_session_files($tmp);
test_sampler($tmp);
done_testing();
