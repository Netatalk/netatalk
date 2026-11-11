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
# Reports and the folder to share, for netatalk-diag.
#
# netatalk-diag collects; this helper analyses what it collected, takes every
# name and address out, and checks the result before netatalk-diag packages
# it. It also samples a Linux process, times a read-only walk of the volumes,
# copies the files the session writes for -m, and holds the D and bpftrace
# programs. It needs Perl 5.14 or later and no module beyond Perl's own.
#
# usage: netatalk_diag.pl report OUTDIR FOLDER [--force]
#        netatalk_diag.pl pcap CAPTURE [--csv FILE] [--keep-names] [--port N]
#        netatalk_diag.pl volumes AFPCONF MOUNTS
#        netatalk_diag.pl sample PID FILE
#        netatalk_diag.pl baseline AFPCONF ENTRIES
#        netatalk_diag.pl session-files PID UID logs|dumps [--since FILE]
#                         [--copy FOLDER [--suffix SUFFIX]]
#        netatalk_diag.pl program NAME [--pid PID] [--afpd PATH]

# root runs this: no module from the working folder, which perls before 5.26
# search last
BEGIN {
    @INC = grep { $_ ne '.' } @INC;
}

package NetatalkDiag;

use strict;
use warnings;
use feature 'unicode_strings';
use sort 'stable';

use Config       qw(%Config);
use Cwd          ();
use Digest::SHA  ();
use Encode       ();
use Fcntl        qw(O_NOFOLLOW O_NONBLOCK O_RDONLY S_ISDIR S_ISREG);
use Getopt::Long ();
use List::Util   qw(first max min sum);
use POSIX        ();
use Scalar::Util qw(looks_like_number);
use Time::HiRes  ();

use constant {
              SYN       => 0x02,
              RST       => 0x04,
              ACK       => 0x10,
              SEQ_SPACE => 4294967296,

              # segments that may wait behind a lost one before filler takes its place
              HOLE_SEGMENTS => 256,

              # the largest DSI frame, netatalk's largest server quantum: resync() looks
              # that far for frames that line up, and a hole beyond it is a lost stream
              MAX_FRAME => 256 * 1024 * 1024,

              # the bytes of a payload the reports read
              HEAD => 65536,
};

my $MARK = 'navmark-';

my %AFP_CMDS = map { split /:/ } qw(
    1:FPByteRangeLock 2:FPCloseVol 3:FPCloseDir 4:FPCloseFork 5:FPCopyFile 6:FPCreateDir 7:FPCreateFile
    8:FPDelete 9:FPEnumerate 10:FPFlush 11:FPFlushFork 14:FPGetForkParms 15:FPGetSrvrInfo 16:FPGetSrvrParms
    17:FPGetVolParms 18:FPLogin 19:FPLoginCont 20:FPLogout 21:FPMapID 22:FPMapName 23:FPMoveAndRename
    24:FPOpenVol 25:FPOpenDir 26:FPOpenFork 27:FPRead 28:FPRename 29:FPSetDirParms 30:FPSetFileParms
    31:FPSetForkParms 32:FPSetVolParms 33:FPWrite 34:FPGetFileDirParms 35:FPSetFileDirParms
    36:FPChangePassword 37:FPGetUserInfo 38:FPGetSrvrMsg 39:FPCreateID 40:FPDeleteID 41:FPResolveID
    42:FPExchangeFiles 43:FPCatSearch 48:FPOpenDT 49:FPCloseDT 51:FPGetIcon 52:FPGetIconInfo 53:FPAddAPPL
    54:FPRemoveAPPL 55:FPGetAPPL 56:FPAddComment 57:FPRemoveComment 58:FPGetComment 59:FPByteRangeLockExt
    60:FPReadExt 61:FPWriteExt 62:FPGetAuthMethods 63:FPLoginExt 64:FPGetSessionToken
    65:FPDisconnectOldSession 66:FPEnumerateExt 67:FPCatSearchExt 68:FPEnumerateExt2 69:FPGetExtAttr
    70:FPSetExtAttr 71:FPRemoveExtAttr 72:FPListExtAttrs 73:FPGetACL 74:FPSetACL 75:FPAccess
    76:FPSpotlightRPC 78:FPSyncDir 79:FPSyncFork 122:FPZzzzz 192:FPAddIcon
);

my %DSI_CMDS = map { split /:/ } qw(
    1:DSICloseSession 2:DSICommand 3:DSIGetStatus 4:DSIOpenSession 5:DSITickle 6:DSIWrite 8:DSIAttention
);

# the requests a client sends in DSI frames of their own
my %DSI_REQUESTS = map { $_ => $DSI_CMDS{$_} } 1, 3, 4;

# names the Finder makes itself, which no token needs to hide
my %KEEP_NAMES = map { $_ => 1 } (
                                  "Icon\r",                              '.DS_Store',
                                  '.Trashes',                            '.TemporaryItems',
                                  'Temporary Items',                     'Network Trash Folder',
                                  '.Spotlight-V100',                     '.fseventsd',
                                  '.metadata_never_index',               '.VolumeIcon.icns',
                                  '.com.apple.timemachine.donotpresent', '.localized',
);

# offset of the path type byte in the AFP request, for requests that carry a
# volume, a DID and a path; the DID sits at offset 4 in all of them
my %PATH_AT = (
               3  => undef, 6  => 8,  7  => 8,  8  => 8,  9  => 18, 23 => undef, 25 => 8,  26 => 12, 28 => 8,
               29 => 10,    30 => 10, 34 => 12, 35 => 10, 66 => 18, 68 => 22,    69 => 30, 72 => 20, 73 => 14,
               75 => 30,    78 => undef,
);

# requests with a file bitmap at offset 8 and a directory bitmap at 10
my %BITMAPS_AT = map { $_ => 1 } 9, 34, 66, 68;

# requests whose fork reference number follows the command and flag bytes
my %FORK_AT = map { $_ => 1 } 1,  4,  11, 14, 27, 31, 33, 59, 60, 61, 79;
my %LOGINS  = map { $_ => 1 } 18, 19, 63;
my @SIZES   = ([4096, '<4K'], [65536, '<64K'], [262144, '<256K'], [1048576, '<1M'], [undef, '>=1M']);

# ---- text, files and tallies ----

sub dec {
    my ($bytes) = @_;
    return Encode::decode('UTF-8', $bytes);
}

sub enc {
    my ($text) = @_;
    return Encode::encode('UTF-8', $text);
}

# a file's text, or undef; bytes that are not UTF-8 read U+FFFD
sub read_text {
    my ($path) = @_;
    return undef if -d $path;
    open(my $fh, '<:raw', $path) or return undef;
    my $bytes = do { local $/; <$fh> };
    close $fh;
    return dec($bytes // '');
}

sub write_text {
    my ($path, $text) = @_;
    open(my $fh, '>:raw', $path) or die "netatalk-diag: $path: $!\n";
    print {$fh} enc($text);
    close $fh or die "netatalk-diag: $path: $!\n";
    return;
}

sub list_dir {
    my ($dir) = @_;
    opendir(my $dh, $dir) or return ();
    my @names = grep { $_ ne '.' && $_ ne '..' } readdir $dh;
    closedir $dh;
    return sort @names;
}

sub trim {
    my ($s) = @_;
    $s =~ s/\A\s+|\s+\z//g;
    return $s;
}

# s from offset from to offset to, both clamped to the string
sub sl {
    my ($s, $from, $to) = @_;
    my $n = length $s;
    $to = $n if !defined $to || $to > $n;
    return $from < $to ? substr($s, $from, $to - $from) : '';
}

# the lines of a text, split at every Unicode line break; a text that ends
# with one has no empty line after it
sub lines {
    my ($text) = @_;
    return () if !defined $text || $text eq '';
    my @lines = split /\r\n|[\n\r\x0b\x0c\x1c-\x1e\x{85}\x{2028}\x{2029}]/, $text, -1;
    pop @lines if $lines[-1] eq '';
    return @lines;
}

# the nearest-rank p-th percentile of values, or undef
sub pct {
    my ($values, $p) = @_;
    return undef unless @{$values};
    my @v = sort { $a <=> $b } @{$values};
    my $i = POSIX::ceil($p / 100 * @v);
    $i = @v if $i > @v;
    $i = 0  if $i < 0;
    return $v[$i - 1];
}

# a tally that keeps the order its keys first came in, for most_common()
sub counter {
    return {n => {}, order => []};
}

sub bump {
    my ($c, $key, $by) = @_;
    push @{$c->{order}}, $key unless exists $c->{n}{$key};
    $c->{n}{$key} += $by // 1;
    return;
}

# the keys by count, the highest first, ties in the order they first came
sub most_common {
    my ($c, $limit) = @_;
    my @keys = sort { $c->{n}{$b} <=> $c->{n}{$a} } @{$c->{order}};
    splice(@keys, $limit) if defined $limit && @keys > $limit;
    return @keys;
}

# [key, [items]] for each key, in the order each key first comes
sub group_by {
    my ($key_of, @items) = @_;
    my (@groups, %at);
    for my $item (@items) {
        my $k = $key_of->($item);
        $at{$k} //= push(@groups, [$k, []]) - 1;
        push @{$groups[$at{$k}][1]}, $item;
    }
    return @groups;
}

# replace names by per-run tokens, one token per distinct name, and keep every
# name replaced for the leak check
sub new_names {
    my ($keep) = @_;
    return {keep => $keep, key => random_key(), seen => {}};
}

sub token {
    my ($names, $name) = @_;
    return $name if $names->{keep} || $KEEP_NAMES{$name} || index($name, $MARK) == 0;
    $names->{seen}{$name} = 1;
    return 'n' . substr(Digest::SHA::hmac_sha256_hex(enc($name), $names->{key}), 0, 10);
}

sub random_key {
    open(my $fh, '<:raw', '/dev/urandom') or die "netatalk-diag: /dev/urandom: $!\n";
    my $n = read($fh, my $key, 16);
    close $fh;
    die "netatalk-diag: /dev/urandom gave no key\n" unless ($n // 0) == 16;
    return $key;
}

# numbered labels for the users and clients of a run
sub new_labels {
    my ($prefix) = @_;
    return {prefix => $prefix, map => {}};
}

sub label {
    my ($labels, $value) = @_;
    my $map = $labels->{map};
    if (!exists $map->{$value}) {
        my $n = 1 + keys %{$map};
        $map->{$value} = $labels->{prefix} . $n;
    }
    return $map->{$value};
}

# ---- the capture ----

# call each(link type, timestamp, link payload) for each record of a classic
# pcap file
sub read_pcap {
    my ($path, $each) = @_;
    open(my $fh, '<:raw', $path) or die "netatalk-diag: $path: $!\n";
    return if (read($fh, my $hdr, 24) // 0) < 24;
    my $magic = unpack('V', $hdr);
    my $u32;
    if ($magic == 0xa1b2c3d4 || $magic == 0xa1b23c4d) {
        $u32 = 'V';
    } elsif ($magic == 0xd4c3b2a1 || $magic == 0x4d3cb2a1) {
        $u32 = 'N';
    } else {
        die "not a classic pcap file (pcapng? convert with 'tcpdump -r in -w out')\n";
    }
    my $frac     = ($magic == 0xa1b23c4d || $magic == 0x4d3cb2a1) ? 1e9 : 1e6;
    my $linktype = unpack($u32, substr($hdr, 20, 4));
    while ((read($fh, my $rec, 16) // 0) == 16) {
        my ($sec, $sub, $incl) = unpack("$u32$u32$u32", $rec);

        # the capture was cut off
        last if (read($fh, my $data, $incl) // 0) < $incl;
        $each->($linktype, $sec + $sub / $frac, $data);
    }
    close $fh;
    return;
}

# the address families of IPv4 and of IPv6 on the BSDs, in a loopback header
my %INET = map { $_ => 1 } 2, 24, 28, 30;

# the IP packet inside a link-layer frame, or undef
sub ip_payload {
    my ($linktype, $data) = @_;
    my $len = length $data;
    if ($linktype == 1) {    # Ethernet
        return undef if $len < 14;
        my ($off, $etype) = (14, unpack('n', substr($data, 12, 2)));
        while ($etype == 0x8100 || $etype == 0x88a8) {    # VLAN tags
            return undef if $len < $off + 4;
            $etype = unpack('n', substr($data, $off + 2, 2));
            $off += 4;
        }
        return ($etype == 0x0800 || $etype == 0x86dd) ? sl($data, $off) : undef;
    }
    if ($linktype == 0) {    # BSD loopback
        return undef if $len < 4;
        my $family = unpack('V', $data);
        $family = unpack('N', $data) unless $INET{$family};
        return $INET{$family} ? sl($data, 4) : undef;
    }
    return sl($data, 16) if $linktype == 113;                                  # Linux cooked
    return sl($data, 20) if $linktype == 276;                                  # Linux cooked v2
    return $data if $linktype == 101 || $linktype == 12 || $linktype == 14;    # raw IP
    return undef;
}

# the MSS and window scale a SYN offers
sub tcp_options {
    my ($buf) = @_;
    my ($n, $i, %opts) = (length $buf, 0);
    while ($i < $n) {
        my $kind = ord substr($buf, $i, 1);
        last if $kind == 0;
        if ($kind == 1) {
            $i++;
            next;
        }
        last if $i + 1 >= $n;
        my $len = ord substr($buf, $i + 1, 1);
        last if $len < 2;
        if ($kind == 2 && $len == 4 && $i + 4 <= $n) {
            $opts{mss} = unpack('n', substr($buf, $i + 2, 2));
        } elsif ($kind == 3 && $len == 3 && $i + 3 <= $n) {
            $opts{wscale} = min(ord substr($buf, $i + 2, 1), 14);
        }
        $i += $len;
    }
    return \%opts;
}

# the TCP segment of an IP packet, or undef
sub tcp_segment {
    my ($ip) = @_;
    return undef if !defined $ip || $ip eq '';
    my $version = ord($ip) >> 4;
    my ($src, $dst, $tcp);
    if ($version == 4) {
        my $ihl = (ord($ip) & 0x0f) * 4;
        return undef if length($ip) < 20 || $ihl < 20 || ord(substr($ip, 9, 1)) != 6;
        my $total = unpack('n', substr($ip, 2, 2));
        ($src, $dst) = (substr($ip, 12, 4), substr($ip, 16, 4));
        $tcp = $total >= $ihl ? sl($ip, $ihl, $total) : sl($ip, $ihl);
    } elsif ($version == 6) {
        return undef if length($ip) < 40;
        my $plen = unpack('n', substr($ip, 4, 2));
        ($src, $dst) = (substr($ip, 8, 16), substr($ip, 24, 16));
        my ($nxt, $off) = (ord(substr($ip, 6, 1)), 40);

        # hop-by-hop, routing and destination options carry their length; a
        # fragment header is eight bytes
        while ($nxt == 0 || $nxt == 43 || $nxt == 44 || $nxt == 60) {
            return undef if length($ip) < $off + 2;
            my $step = $nxt == 44 ? 8 : (ord(substr($ip, $off + 1, 1)) + 1) * 8;
            ($nxt, $off) = (ord(substr($ip, $off, 1)), $off + $step);
        }
        return undef if $nxt != 6;
        $tcp = sl($ip, $off, 40 + $plen);
    } else {
        return undef;
    }
    return undef if length($tcp) < 20;
    my ($sport, $dport, $seq, $ack, $doff, $flags, $window) = unpack('n n N N C C n', $tcp);
    $doff = ($doff >> 4) * 4;
    return {
            src     => $src,
            dst     => $dst,
            sport   => $sport,
            dport   => $dport,
            seq     => $seq,
            ack     => $ack,
            flags   => $flags,
            window  => $window,
            opts    => ($flags & SYN) ? tcp_options(sl($tcp, 20, $doff)) : {},
            payload => sl($tcp, $doff),
    };
}

# the offset of sequence number seq from base, modulo 2^32
sub seq_off {
    my ($seq, $base) = @_;
    return ($seq - $base) % SEQ_SPACE;
}

# one direction of a TCP connection, in order, with byte timestamps: base is
# the sequence number of data's first byte, ends and times each appended
# segment's end offset and timestamp, pending the out-of-order segments by
# offset, and holes the offsets where filler stands for lost bytes
sub new_stream {
    return {data => '', base => undef, ends => [], times => [], pending => {}, gaps => 0, holes => {}};
}

sub stream_add {
    my ($st, $seq, $flags, $payload, $ts) = @_;
    if (!defined $st->{base}) {
        $st->{base} = ($flags & SYN) ? ($seq + 1) % SEQ_SPACE : $seq;
        return if $flags & SYN;
    }
    return if $payload eq '';
    my $off = seq_off($seq, $st->{base});

    # before the stream's start: a retransmission
    return if $off > 0x7fffffff;

    # what waits after a drain waits behind a hole, which a later segment
    # leaves open until enough wait behind it
    my $blocked = %{$st->{pending}};
    $st->{pending}{$off} //= [$payload, $ts];
    return if $blocked && $off > length $st->{data} && keys %{$st->{pending}} < HOLE_SEGMENTS;
    stream_drain($st, 0);
    return;
}

sub stream_drain {
    my ($st, $fill) = @_;
    my $pending = $st->{pending};
    while (%{$pending}) {
        my $have  = length $st->{data};
        my @ready = grep { $_ <= $have } keys %{$pending};
        if (!@ready) {

            # a segment the capture lost: once enough wait behind it, or at
            # the end, 0xff bytes take its place, so later bytes keep their
            # offsets, and dsi_frames() resyncs past them. No DSI header starts
            # with 0xff or holds it in its reserved word, so the search for one
            # skips the filler in a few steps.
            return if !$fill && keys %{$pending} < HOLE_SEGMENTS;
            my $nxt = min(keys %{$pending});
            if ($nxt - $have > MAX_FRAME) {

                # a jump no frame spans: the stream is lost from here
                %{$pending} = ();
                $st->{gaps}++;
                return;
            }
            $st->{holes}{$have} = 1;
            $st->{data} .= "\xff" x ($nxt - $have);
            push @{$st->{ends}},  length $st->{data};
            push @{$st->{times}}, $pending->{$nxt}[1];
            $st->{gaps}++;
            next;
        }
        for my $o (sort { $a <=> $b } @ready) {
            my ($payload, $ts) = @{delete $pending->{$o}};
            my $tail = $o < $have ? sl($payload, $have - $o) : $payload;
            if ($tail ne '') {
                $st->{data} .= $tail;
                push @{$st->{ends}},  length $st->{data};
                push @{$st->{times}}, $ts;
            }
            $have = length $st->{data};
        }
    }
    return;
}

sub stream_finish {
    my ($st) = @_;
    stream_drain($st, 1);
    return;
}

# the timestamp of the segment that carried byte offset; the frames come in
# order, so the search goes on from the segment of the last answer
sub time_at {
    my ($st,   $offset) = @_;
    my ($ends, $times)  = ($st->{ends}, $st->{times});
    my $i = $st->{at} // 0;
    if ($i > 0 && $ends->[$i - 1] > $offset) {
        my $lo = 0;
        while ($lo < $i) {
            my $mid = int(($lo + $i) / 2);
            if ($ends->[$mid] <= $offset) {
                $lo = $mid + 1;
            } else {
                $i = $mid;
            }
        }
    } else {
        $i++ while $i < @{$ends} && $ends->[$i] <= $offset;
    }
    $st->{at} = $i;
    return $times->[min($i, $#{$times})];
}

# what one end of a connection sent, as the server's capture saw it: base is
# the sequence number of its first data byte, top the offset past the highest
# byte it sent, acked the offset the other end acknowledged, window its latest
# window as the header has it, syn when it sent its SYN, unacked the [end,
# sent] of new data, rtts the delays from new data to its first ACK, and
# at_window the sends that filled the other end's window
sub new_side {
    return {
            base         => undef,
            top          => 0,
            acked        => 0,
            segments     => 0,
            bytes        => 0,
            retrans      => 0,
            zero_windows => 0,
            window       => undef,
            mss          => undef,
            wscale       => undef,
            syn          => undef,
            unacked      => [],
            rtts         => [],
            sends        => 0,
            at_window    => 0,
    };
}

# count a segment me sent, and the ACK it carries for peer's data
sub account {
    my ($me, $peer, $seg, $ts) = @_;
    my $flags = $seg->{flags};
    if ($flags & SYN) {
        $me->{syn}    = $ts;
        $me->{mss}    = $seg->{opts}{mss};
        $me->{wscale} = $seg->{opts}{wscale};
        $me->{base}   = ($seg->{seq} + 1) % SEQ_SPACE;
    } elsif (!defined $me->{base}) {
        $me->{base} = $seg->{seq};
    }
    if (!($flags & RST)) {
        $me->{window} = $seg->{window};
        $me->{zero_windows}++ if $seg->{window} == 0 && !($flags & SYN);
    }
    if (($flags & ACK) && defined $peer->{base}) {
        my $acked = seq_off($seg->{ack}, $peer->{base});
        if ($acked <= 0x7fffffff && $acked > $peer->{acked}) {
            $peer->{acked} = $acked;
            my $unacked = $peer->{unacked};
            my $last;
            $last = shift @{$unacked} while @{$unacked} && $unacked->[0][0] <= $acked;
            push @{$peer->{rtts}}, $ts - $last->[1] if $last;
        }
    }
    my $n = length $seg->{payload};
    return if !$n || ($flags & SYN);
    my $off = seq_off($seg->{seq}, $me->{base});
    $me->{segments}++;
    $me->{bytes} += $n;
    if ($off > 0x7fffffff || $off + $n <= $me->{top}) {

        # sent before, or before the capture began; an ACK after a
        # retransmission times neither copy
        $me->{retrans}++;
        $me->{unacked} = [grep { $_->[0] <= $off } @{$me->{unacked}}];
        return;
    }
    $me->{top} = $off + $n;
    push @{$me->{unacked}}, [$me->{top}, $ts];
    $me->{sends}++;

    # window scaling holds only when both SYNs offered it
    if (defined $peer->{window} && defined $peer->{wscale} && defined $me->{wscale}) {
        my $room = ($peer->{window} << $peer->{wscale}) - ($me->{top} - $me->{acked});
        $me->{at_window}++ if $room < ($me->{mss} || 1460);
    }
    return;
}

# the DSI header fields at pos of the stream data, or undef where no header
# can start
sub frame_at {
    my ($data, $pos) = @_;
    my $len = length ${$data};
    return undef if $pos + 16 > $len;
    my ($flags, $cmd, $reqid, $code, $length, $reserved) = unpack('C C n l> N N', substr(${$data}, $pos, 16));
    return undef if $flags > 1 || !$DSI_CMDS{$cmd} || $reserved != 0 || $length > MAX_FRAME;

    # a request's payload starts with an AFP command
    return undef
      if $flags == 0
      && ($cmd == 2    || $cmd == 6)
      && ($length == 0 || ($pos + 16 < $len && !$AFP_CMDS{ord substr(${$data}, $pos + 16, 1)}));
    return [$flags, $cmd, $reqid, $code, $length];
}

# a DSI header: flags 0 or 1, a DSI command, a length below MAX_FRAME and the
# reserved word 0, which the regex engine finds first
my $HEADER = qr/[\x00\x01][\x01-\x06\x08][\x00-\xff]{6}[\x00-\x10][\x00-\xff]{3}\x00{4}/;

# the first offset from pos where three frames follow one another, or frames
# that end at the capture's end or at a hole, or whole frames and then one the
# capture cut
sub resync {
    my ($data, $pos, $holes) = @_;
    my $len = length ${$data};
    my $end = min($len, $pos + MAX_FRAME);
    pos(${$data}) = $pos;
    while (${$data} =~ /$HEADER/g) {
        my $start = $-[0];
        last if $start + 16 > $end;

        # headers may overlap: the next search starts one byte on
        pos(${$data}) = $start + 1;
        my ($p, $whole) = ($start, 0);
        while ($whole < 3) {
            my $f = frame_at($data, $p) or last;
            $p += 16 + $f->[4];
            if ($p > $len) {
                $whole = 3 if $whole;
                last;
            }
            $whole++;
            $whole = 3 if $p == $len || $holes->{$p};
        }
        return $start if $whole >= 3;
    }
    return undef;
}

# call each(start, end, flags, command, reqid, code, payload) for each frame,
# the payload cut at HEAD bytes: a request's command and path and a reply's
# first fields are all the reports read, and a transfer's data is not copied.
#
# A capture that starts inside a session, or that lost bytes, resumes at the
# next offset where frames line up.
sub dsi_frames {
    my ($st, $each) = @_;
    my $data = \$st->{data};
    my $len  = length ${$data};
    my $pos  = resync($data, 0, $st->{holes});
    while (defined $pos && $pos + 16 <= $len) {
        my $f = frame_at($data, $pos);
        if (!$f) {
            $st->{gaps}++;
            $pos = resync($data, $pos + 1, $st->{holes});
            next;
        }
        my ($flags, $cmd, $reqid, $code, $length) = @{$f};
        my $end = $pos + 16 + $length;

        # the capture ended inside the frame
        return if $end > $len;
        $each->($pos, $end, $flags, $cmd, $reqid, $code, substr(${$data}, $pos + 16, min($length, HEAD)));
        $pos = $end;
    }
    return;
}

# (names, hops, text) of the AFP path whose type byte is at offset at.
#
# As the server reads it: the first NUL of a run separates two names, and each
# further NUL of the run climbs to the parent. tokens gives each name the text
# shows.
sub decode_path {
    my ($buf, $at, $tokens) = @_;
    return (0, 0, '') if !defined $at || $at >= length $buf;
    my $ptype = ord substr($buf, $at, 1);
    my $raw;
    if ($ptype == 3 && $at + 7 <= length $buf) {
        $raw = sl($buf, $at + 7, $at + 7 + unpack('n', substr($buf, $at + 5, 2)));
    } elsif (($ptype == 1 || $ptype == 2) && $at + 2 <= length $buf) {
        $raw = sl($buf, $at + 2, $at + 2 + ord substr($buf, $at + 1, 1));
    } else {
        return (0, 0, '');
    }
    my ($names, $hops, $i, $n, @out) = (0, 0, 0, length $raw);
    while ($i < $n) {
        if (substr($raw, $i, 1) eq "\0") {
            my $run = 0;
            while ($i < $n && substr($raw, $i, 1) eq "\0") {
                $run++;
                $i++;
            }
            $hops += $run - 1;
            push @out, ('..') x ($run - 1);
            next;
        }
        my $j = index($raw, "\0", $i);
        $j = $n if $j < 0;
        (my $name = token($tokens, dec(substr($raw, $i, $j - $i)))) =~ s/\r/\\r/g;
        push @out, $name;
        $names++;
        $i = $j;
    }
    return ($names, $hops, join('/', @out));
}

# each TCP connection to port, in the order of its first packet: both streams
# and both ends' TCP figures
sub read_connections {
    my ($path, $port) = @_;
    my (%conns, @order);
    my $each = sub {
        my ($linktype, $ts, $data) = @_;
        my $seg = tcp_segment(ip_payload($linktype, $data)) or return;
        my $c2s = $seg->{dport} == $port;
        return unless $c2s || $seg->{sport} == $port;
        my $side = $c2s ? 'c2s' : 's2c';

        # the client's address and port, then the server's
        my @ends = ([@{$seg}{qw(src sport)}], [@{$seg}{qw(dst dport)}]);
        my $key  = join('|', map { unpack('H*', $_->[0]), $_->[1] } $c2s ? @ends : reverse @ends);

        # a client SYN on a four-tuple that carried data opens a new
        # connection: the old one keeps its label under another key
        if ($c2s && ($seg->{flags} & SYN) && !($seg->{flags} & ACK) && $conns{$key} && defined $conns{$key}{c2s}{base})
        {
            my $old = "$key|" . scalar @order;
            $conns{$old} = delete $conns{$key};
            @order = ((grep { $_ ne $key } @order), $old);
        }
        my $conn = $conns{$key};
        if (!$conn) {
            $conn = $conns{$key} = {
                                    c2s         => new_stream(),
                                    s2c         => new_stream(),
                                    client      => new_side(),
                                    server      => new_side(),
                                    label       => 'c' . (@order + 1),
                                    established => undef,
            };
            push @order, $key;
        }
        if ($c2s) {
            account($conn->{client}, $conn->{server}, $seg, $ts);
            $conn->{established} = $ts
              if !defined $conn->{established}
              && defined $conn->{server}{syn}
              && ($seg->{flags} & ACK)
              && !($seg->{flags} & SYN);
        } else {
            account($conn->{server}, $conn->{client}, $seg, $ts);
        }
        stream_add($conn->{$side}, $seg->{seq}, $seg->{flags}, $seg->{payload}, $ts);
    };
    read_pcap($path, $each);
    my @conns = map { $conns{$_} } @order;
    for my $conn (@conns) {
        stream_finish($conn->{c2s});
        stream_finish($conn->{s2c});
    }
    return \@conns;
}

sub new_row {
    my ($cmd) = @_;
    return {
            cmd         => $cmd,
            afp         => undef,
            vol         => undef,
            did         => undef,
            names       => 0,
            hops        => 0,
            path        => '',
            bitmaps     => '',
            refnum      => undef,
            fork        => undef,
            offset      => undef,
            count       => undef,
            data_bytes  => 0,
            t_reply     => undef,
            t_reply_end => undef,
            result      => undef,
            reply_bytes => undef,
    };
}

# a signed 64-bit big-endian integer, which a 32-bit perl reads too
sub s64 {
    my ($bytes) = @_;
    my ($hi, $lo) = unpack('l> N', $bytes);
    return $hi * 4294967296 + $lo;
}

# a row for the AFP request in payload, of length bytes in all
sub afp_request {
    my ($payload, $length, $cmd, $code, $tokens) = @_;
    my $afp = ord $payload;
    my $n   = length $payload;
    my $row = new_row($AFP_CMDS{$afp} // "AFP$afp");
    $row->{afp} = $afp;
    @{$row}{qw(vol did)}         = unpack('n N', substr($payload, 2, 6)) if exists $PATH_AT{$afp} && $n >= 8;
    @{$row}{qw(names hops path)} = decode_path($payload, $PATH_AT{$afp}, $tokens);
    $row->{bitmaps} = sprintf('f%04x d%04x', unpack('n n', substr($payload, 8, 4))) if $BITMAPS_AT{$afp} && $n >= 12;
    $row->{refnum}  = unpack('n', substr($payload, 2, 2))                           if $FORK_AT{$afp}    && $n >= 4;
    $row->{fork}    = (ord(substr($payload, 1, 1)) & 0x80) ? 'rsrc' : 'data'        if $afp == 26        && $n >= 2;

    if (($afp == 60 || $afp == 61) && $n >= 20) {
        @{$row}{qw(offset count)} = (s64(substr($payload, 4, 8)), s64(substr($payload, 12, 8)));
    } elsif (($afp == 27 || $afp == 33) && $n >= 12) {
        @{$row}{qw(offset count)} = unpack('l> l>', substr($payload, 4, 8));
    }

    # a DSIWrite's code is where its data starts
    $row->{data_bytes} = max(0, $length - $code) if $cmd == 6;
    return $row;
}

# the requests of one connection, each with its reply
sub connection_requests {
    my ($conn,    $tokens) = @_;
    my ($s2c,     $c2s)    = @{$conn}{qw(s2c c2s)};
    my (%replies, @rows);
    $conn->{server_requests} = {};
    $conn->{tickles}         = 0;
    my $reply = sub {
        my ($start, $end, $flags, $cmd, $reqid, $code, $payload) = @_;
        if ($flags == 1) {
            push @{$replies{$reqid}},
              [time_at($s2c, $start), time_at($s2c, $end - 1), $code, $end - $start - 16, substr($payload, 0, 4)];
        } else {
            $conn->{server_requests}{$DSI_CMDS{$cmd} // "DSI$cmd"}++;
        }
    };
    my $request = sub {
        my ($start, $end, $flags, $cmd, $reqid, $code, $payload) = @_;
        return if $flags != 0;
        if ($cmd == 5) {
            $conn->{tickles}++;
            return;
        }
        my $row;
        if (($cmd == 2 || $cmd == 6) && $payload ne '') {
            $row = afp_request($payload, $end - $start - 16, $cmd, $code, $tokens);
        } elsif ($DSI_REQUESTS{$cmd}) {
            $row = new_row($DSI_REQUESTS{$cmd});
        } else {
            return;
        }
        my $t_req = time_at($c2s, $end - 1);
        @{$row}{qw(conn t t_req_end)} = ($conn->{label}, time_at($c2s, $start), $t_req);
        my $list = $replies{$reqid} || [];
        for my $i (0 .. $#{$list}) {
            next if $list->[$i][0] < $t_req;
            @{$row}{qw(t_reply t_reply_end result reply_bytes head)} = @{splice(@{$list}, $i, 1)};
            last;
        }
        push @rows, $row;
    };
    dsi_frames($s2c, $reply);
    dsi_frames($c2s, $request);

    # a fork's reads and writes name it by the reference its open returned
    my %forks;
    for my $row (@rows) {
        if (    defined $row->{afp}
             && $row->{afp} == 26
             && defined $row->{result}
             && $row->{result} == 0
             && length($row->{head} // '') >= 4) {
            $forks{unpack('n', substr($row->{head}, 2, 2))} = $row->{fork};
        } elsif (defined $row->{refnum}) {
            $row->{fork} = $forks{$row->{refnum}};
            delete $forks{$row->{refnum}} if $row->{afp} == 4;
        }
    }
    return \@rows;
}

# (connections, requests) of a capture, the requests in time order with their
# phase, server time and client gap
sub analyse_capture {
    my ($path, $port, $tokens) = @_;
    my $conns = read_connections($path, $port);
    my @rows  = sort { $a->{t} <=> $b->{t} } map { @{connection_requests($_, $tokens)} } @{$conns};
    my ($phase, %last_end) = ('all');
    for my $r (@rows) {
        my $at = index($r->{path}, $MARK);
        if ($at >= 0) {
            my ($label) = split m{/}, substr($r->{path}, $at + length $MARK);
            $phase = defined $label && $label ne '' ? $label : 'all';
            $r->{marker} = 1;
        }
        $r->{phase} = $phase;
        my $prev  = $last_end{$r->{conn}};
        my $start = defined $prev ? max($r->{t_req_end}, $prev) : $r->{t_req_end};
        $r->{start}           = $start;
        $r->{server_us}       = defined $r->{t_reply}             ? ($r->{t_reply} - $start) * 1e6 : undef;
        $r->{client_gap_us}   = defined $prev && $r->{t} >= $prev ? ($r->{t} - $prev) * 1e6        : undef;
        $last_end{$r->{conn}} = $r->{t_reply_end} if defined $r->{t_reply_end};
    }
    return ($conns, \@rows);
}

sub csv_field {
    my ($v) = @_;
    return '' unless defined $v;
    return $v unless $v =~ /[",\r\n]/;
    $v =~ s/"/""/g;
    return qq{"$v"};
}

sub write_csv {
    my ($rows, $path) = @_;
    my $opt = sub { defined $_[0] ? sprintf('%.0f', $_[0]) : '' };
    my @lines = join(
                     ',',
                     qw(t phase conn cmd result vol did names hops path server_us client_gap_us bitmaps reply_bytes),
                     qw(fork offset count)
    );
    for my $r (@{$rows}) {
        my @f = map { $_ // '' } sprintf('%.6f', $r->{t}),
          @{$r}{qw(phase conn cmd result vol did names hops path)},
          $opt->($r->{server_us}),
          $opt->($r->{client_gap_us}),
          @{$r}{qw(bitmaps reply_bytes fork)},
          $opt->($r->{offset}),
          $opt->($r->{count});
        my $line = join(',', @f);

        # a field holds a comma, a quote or a line break only in a rare path
        $line = join(',', map { csv_field($_) } @f) if ($line =~ tr/,//) != $#f || $line =~ /["\r\n]/;
        push @lines, $line;
    }
    write_text($path, join('', map {"$_\r\n"} @lines));
    return;
}

sub phases_of {
    my ($rows) = @_;
    return group_by(sub { $_[0]{phase} }, grep { !$_->{marker} } @{$rows});
}

sub phase_wall {
    my ($rs) = @_;
    return max(map { $_->{t_reply_end} || $_->{t} } @{$rs}) - $rs->[0]{t};
}

sub server_time {
    my ($rs) = @_;
    return sum(0, map { $_->{server_us} || 0 } @{$rs});
}

# per phase: requests and server time per AFP command, the paths' shape, the
# DIDs asked for most and the slowest requests
sub report_requests {
    my ($rows, $out) = @_;
    my @phases = phases_of($rows);
    push @{$out}, 'no AFP requests found' unless @phases;
    for my $ph (@phases) {
        my ($phase, $rs) = @{$ph};
        push @{$out},
          sprintf(
                  '== phase %s: %d requests, %.3f s wall, %.3f s server, %.3f s client gaps, %d unanswered',
                  $phase,
                  scalar @{$rs},
                  phase_wall($rs),
                  server_time($rs) / 1e6,
                  sum(0, map { $_->{client_gap_us} || 0 } @{$rs}) / 1e6,
                  scalar grep { !defined $_->{server_us} } @{$rs}
          );
        push @{$out}, sprintf('   %-20s %6s %10s %9s %9s %7s', qw(command n server_ms mean_us max_us errors));
        my @by = group_by(sub { $_[0]{cmd} }, @{$rs});
        for my $g (sort { server_time($b->[1]) <=> server_time($a->[1]) } @by) {
            my ($cmd, $cr) = @{$g};
            my @lat  = grep {defined} map { $_->{server_us} } @{$cr};
            my $errs = grep { defined $_->{result} && $_->{result} != 0 } @{$cr};
            push @{$out},
              sprintf(
                      '   %-20s %6d %10.1f %9.0f %9.0f %7d',
                      $cmd, scalar @{$cr}, sum(0, @lat) / 1e3, @lat ? sum(@lat) / @lat : 0, @lat ? max(@lat) : 0, $errs
              );
        }
        my $hist = counter();
        bump($hist, "$_->{names} $_->{hops}") for grep { $_->{path} ne '' || defined $_->{did} } @{$rs};
        if (@{$hist->{order}}) {
            my @shapes = map { [split ' ', $_] } @{$hist->{order}};
            push @{$out},
              '   path shape (names, parent hops): '
              . join(
                     ', ',
                     map  { sprintf('(%d, %d) x%d', @{$_}, $hist->{n}{"@{$_}"}) }
                     sort { $a->[0] <=> $b->[0] || $a->[1] <=> $b->[1] } @shapes
              );
        }
        my $dids = counter();
        bump($dids, $_->{did}) for grep { defined $_->{did} } @{$rs};
        if (@{$dids->{order}}) {
            push @{$out},
              '   most requested DIDs: '
              . join(', ', map { sprintf('%d x%d', $_, $dids->{n}{$_}) } most_common($dids, 8));
        }
        my @slow = sort { $b->{server_us} <=> $a->{server_us} } grep { defined $_->{server_us} } @{$rs};
        splice(@slow, 5) if @slow > 5;
        for my $r (@slow) {
            push @{$out},
              sprintf(
                      "   slow: %8.0f us %-18s did %s path '%s'%s",
                      $r->{server_us}, $r->{cmd}, $r->{did} // '-', $r->{path},
                      $r->{bitmaps} ne '' ? " bitmaps $r->{bitmaps}" : ''
              );
        }
    }
    return;
}

my @PARTS = (
             'afpd working on requests',
             'replies on their way to the client',
             'written data arriving',
             "afpd waiting for the client's next request",
);
my @VERDICTS = (
            'afpd set the pace: it worked on requests %.0f%% of the time; the request table and a -t trace show where',
            'sending set the pace: replies were on their way %.0f%% of the time; see the TCP section',
            "the client's writes set the pace: written data was arriving %.0f%% of the time; see the TCP section",
            'the client set the pace: afpd waited for its next request %.0f%% of the time',
);

# per phase and connection: where the time went, and what set the pace. afpd
# serves a connection's requests one at a time, so the server's work on one
# runs from the later of its arrival and the previous reply's end to its
# reply's first byte, and the reply's bytes go out after it.
sub verdicts {
    my ($rows, $out) = @_;
    for my $ph (phases_of($rows)) {
        for my $g (group_by(sub { $_[0]{conn} }, @{$ph->[1]})) {
            my ($conn, $cr) = @{$g};
            my @done = grep { defined $_->{t_reply_end} } @{$cr};
            next unless @done;
            my $wall = max(map { $_->{t_reply_end} } @done) - $cr->[0]{t};
            my @secs = (
                        sum(0, map { $_->{server_us} } @done) / 1e6,
                        sum(0, map { $_->{t_reply_end} - $_->{t_reply} } @done),
                        sum(0, map { $_->{t_req_end} - $_->{t} } grep { $_->{data_bytes} } @{$cr}),
            );
            push @secs, max(0, $wall - sum(@secs));
            push @{$out},
              sprintf('== phase %s, connection %s: %d requests in %.3f s', $ph->[0], $conn, scalar @{$cr}, $wall);
            my $share = sub { $wall > 0 ? 100 * $_[0] / $wall : 0 };
            push @{$out}, sprintf('   %-46s %9.3f s %4.0f%%', $PARTS[$_], $secs[$_], $share->($secs[$_]))
              for 0 .. $#PARTS;
            my $top = 0;
            for my $i (1 .. $#secs) {
                $top = $i if $secs[$i] > $secs[$top];
            }
            push @{$out}, '   verdict: ' . sprintf($VERDICTS[$top], $share->($secs[$top]));
        }
    }
    return;
}

# per connection: what each end sent and resent, the windows they advertised,
# round trips and the rate of large replies
sub tcp_health {
    my ($conns, $out) = @_;
    for my $conn (@{$conns}) {
        my ($c, $s) = @{$conn}{qw(client server)};
        next unless $c->{segments} || $s->{segments};
        push @{$out}, "== TCP, connection $conn->{label}";
        for my $side (['server', $s], ['client', $c]) {
            my ($name, $x) = @{$side};
            push @{$out},
              sprintf(
                      '   %s sent %.2f MiB in %d data segments, %d retransmitted (%.2f%%), %d zero windows advertised',
                      $name, $x->{bytes} / 1048576, $x->{segments}, $x->{retrans},
                      $x->{segments} ? 100 * $x->{retrans} / $x->{segments} : 0,
                      $x->{zero_windows}
              );
        }
        if (@{$s->{rtts}}) {
            push @{$out},
              sprintf(
                      "   round trip, server data to the client's ACK: median %.2f ms, p90 %.2f ms, %d samples",
                      pct($s->{rtts}, 50) * 1e3,
                      pct($s->{rtts}, 90) * 1e3,
                      scalar @{$s->{rtts}}
              );
        }
        if (defined $conn->{established} && defined $s->{syn}) {
            push @{$out}, sprintf('   handshake round trip: %.2f ms', ($conn->{established} - $s->{syn}) * 1e3);
        }
        if (defined $c->{wscale} && defined $s->{wscale}) {
            push @{$out},
              sprintf(
                      '   MSS: client %s, server %s; window scale: client %d, server %d',
                      $c->{mss} // 'unknown',
                      $s->{mss} // 'unknown',
                      $c->{wscale}, $s->{wscale}
              );
            push @{$out},
              sprintf("   server sends that filled the client's window: %d of %d", $s->{at_window}, $s->{sends});
        } else {
            push @{$out},
              '   the connection began before the capture: window sizes unknown (start the capture before the '
              . 'client connects, -l, to see them)';
        }
    }
    return;
}

# per phase: reads and writes by size, how sequential they were and which
# forks they used, and how many requests the client kept in flight
sub transfers {
    my ($rows, $out) = @_;
    for my $ph (phases_of($rows)) {
        my ($phase, $rs) = @{$ph};
        push @{$out}, "== transfers, phase $phase";
        my $wall = phase_wall($rs);
        for my $kind (['reads', 27, 60], ['writes', 33, 61]) {
            my ($name, @cmds) = @{$kind};
            my %is = map  { $_ => 1 } @cmds;
            my @xs = grep { defined $_->{afp} && $is{$_->{afp}} } @{$rs};
            if (!@xs) {
                push @{$out}, "   $name: none";
                next;
            }
            my @sizes = map { $name eq 'reads' ? $_->{reply_bytes} || 0 : $_->{data_bytes} } @xs;
            my %hist;
            for my $n (@sizes) {
                $hist{(first { !defined $_->[0] || $n < $_->[0] } @SIZES)->[1]}++;
            }

            # a fork's first transfer has nothing to follow
            my ($seq, $later, %last) = (0, 0);
            for my $i (0 .. $#xs) {
                my $r = $xs[$i];
                next unless defined $r->{offset};
                my $key = join(' ', $r->{conn}, $r->{refnum} // '');
                if (exists $last{$key}) {
                    $later++;
                    $seq++ if $last{$key} == $r->{offset};
                }
                $last{$key} = $r->{offset} + $sizes[$i];
            }
            my %forks;
            $forks{$_->{fork} || 'unknown'}++ for @xs;
            my $nforks = keys %last;
            my $mib    = sum(0, @sizes) / 1048576;
            push @{$out},
              sprintf(
                     '   %s: %d on %d fork%s, %.2f MiB, %.1f MiB/s over the phase; sizes %s; sequential %d of %d later '
                   . '%s on a fork; forks %s',
                   $name,
                   scalar @xs,
                   $nforks,
                   $nforks == 1 ? '' : 's',
                   $mib,
                   $wall > 0 ? $mib / $wall : 0,
                   join(', ', map {"$_->[1] $hist{$_->[1]}"} grep { $hist{$_->[1]} } @SIZES),
                   $seq, $later, $name,
                   join(', ', map {"$_ $forks{$_}"} sort keys %forks)
              );
        }
        my @events =
          sort { $a->[0] <=> $b->[0] || $a->[1] <=> $b->[1] }
          map { ([$_->{t}, 1], [$_->{t_reply_end} || $_->{t}, -1]) } @{$rs};
        my ($depth, $top, $busy, $weighted, $prev) = (0, 0, 0, 0);
        for my $e (@events) {
            my ($t, $step) = @{$e};
            if (defined $prev && $depth > 0) {
                $busy     += $t - $prev;
                $weighted += $depth * ($t - $prev);
            }
            $depth += $step;
            $top  = max($top, $depth);
            $prev = $t;
        }
        push @{$out},
          sprintf('   requests in flight: at most %d, %.1f on average while any was', $top,
                  $busy ? $weighted / $busy : 0);
    }
    return;
}

# per connection whose start the capture saw: the times from its first packet
# to the session's open, the login and the volume's open
sub logins {
    my ($conns, $rows, $out) = @_;
    my %by;
    push @{$by{$_->{conn}}}, $_ for @{$rows};
    for my $conn (@{$conns}) {
        my @rs     = @{$by{$conn->{label}} || []};
        my @opened = grep { $_->{cmd} eq 'DSIOpenSession' && defined $_->{t_reply} } @rs;
        next unless @opened;
        my $t0    = $conn->{client}{syn} || $opened[0]{t};
        my @parts = (sprintf('session open %.1f ms', ($opened[0]{t_reply} - $t0) * 1e3));
        my @ls    = grep { defined $_->{afp} && $LOGINS{$_->{afp}} } @rs;
        if (@ls) {
            push @parts,
              sprintf(
                      'login %.1f ms in %d request%s, result %s',
                      (($ls[-1]{t_reply_end} || $ls[-1]{t}) - $ls[0]{t}) * 1e3,
                      scalar @ls, @ls == 1 ? '' : 's', $ls[-1]{result} // 'none'
              );
        }
        my @vols = grep { defined $_->{afp} && $_->{afp} == 24 && defined $_->{t_reply} } @rs;
        push @parts,  sprintf('first volume open %.1f ms', ($vols[0]{t_reply} - $t0) * 1e3) if @vols;
        push @{$out}, "   $conn->{label}, from its first packet: " . join('; ', @parts);
    }
    return;
}

# ---- counters ----

sub is_count {
    my ($s) = @_;
    return defined $s && $s =~ /\A\d+\z/a;
}

# 'name: value' lines as integers
sub load_sysctl {
    my ($text) = @_;
    my %v;
    for my $line (lines($text)) {
        my $i = index($line, ':');
        next if $i < 0;
        my $value = trim(substr($line, $i + 1));
        $v{trim(substr($line, 0, $i))} = 0 + $value if $value =~ /\A[+-]?\d+\z/a;
    }
    return \%v;
}

my @DISK_FIELDS = qw(reads reads_merged read_sectors read_ms writes writes_merged write_sectors write_ms in_flight io_ms
  weighted_ms);
my %ONE_VALUE = map { $_ => 1 } qw(pid.status pid.io vmstat if);

# the /proc files netatalk-diag gathers, after their '== FILE' lines, as
# integers under their file's prefix
sub load_linux {
    my ($text) = @_;
    my (%v, $prefix, %header);
    for my $line (lines($text)) {
        if (substr($line, 0, 3) eq '== ') {
            $prefix = trim(substr($line, 3));
            next;
        }
        my @f = split ' ', $line;
        next if !@f || !defined $prefix;
        if ($prefix eq 'snmp' || $prefix eq 'netstat') {

            # a header line, then its values, each led by the protocol
            if (!$header{$f[0]} || @{$header{$f[0]}} != @f) {
                $header{$f[0]} = \@f;
                next;
            }
            my $head = delete $header{$f[0]};
            for my $i (1 .. $#f) {
                $v{"$f[0]$head->[$i]"} = 0 + $f[$i] if $f[$i] =~ /\A-?\d+\z/a;
            }
        } elsif ($prefix eq 'arcstats' && @f == 3 && is_count($f[2])) {
            $v{"kstat.zfs.misc.arcstats.$f[0]"} = 0 + $f[2];
        } elsif ($prefix eq 'diskstats' && @f >= 14) {
            for my $i (0 .. $#DISK_FIELDS) {
                $v{"disk.$f[2].$DISK_FIELDS[$i]"} = 0 + $f[3 + $i] if is_count($f[3 + $i]);
            }
        } elsif (index($prefix, 'pressure.') == 0) {
            for my $kv (@f[1 .. $#f]) {
                my ($k, $val) = split /=/, $kv, 2;
                $v{"$prefix.$f[0].total_us"} = 0 + $val if $k eq 'total' && is_count($val);
            }
        } elsif ($prefix eq 'pid.stat') {

            # comm may hold spaces: the fields after it count from ")"
            my @rest = split ' ', substr($line, rindex($line, ')') + 1);
            if (@rest > 12 && is_count($rest[11]) && is_count($rest[12])) {
                $v{'pid.utime_ticks'} = 0 + $rest[11];
                $v{'pid.stime_ticks'} = 0 + $rest[12];
            }
        } elsif ($ONE_VALUE{$prefix} && @f >= 2) {
            (my $k = $f[0]) =~ s/:+\z//;
            $v{"$prefix.$k"} = 0 + $f[1] if is_count($f[1]);
        } elsif ($prefix eq 'pid.fds' && is_count($f[0])) {
            $v{'pid.fds'} = 0 + $f[0];
        }
    }
    return \%v;
}

# FreeBSD netstat -s lines, by their wording with digits as N
sub load_netstat_s {
    my ($text) = @_;
    my %v;
    for my $line (lines($text)) {
        my $s = trim($line);
        next unless $s =~ /\A(\d+)/a;
        my $n = $1;
        $s =~ s/\d+/N/ga;
        $v{"tcp $s"} = 0 + $n;
    }
    return \%v;
}

# a header line and rows, keeping the numeric columns of the row whose first
# field is key_col, counted from the right
sub load_table {
    my ($text, $key_col) = @_;
    my @lines = map { [split ' ', $_] } grep {/\S/} lines($text);
    return {} if @lines < 2;
    my @head = @{shift @lines};

    # the link-level row counts every address family
    my @rows = sort { link_row($b) <=> link_row($a) } grep { $_->[0] eq $key_col } @lines;
    return {} unless @rows;
    my @f = @{$rows[0]};
    my %v;
    for my $i (1 .. min(scalar @head, scalar @f)) {
        last unless is_count($f[-$i]);
        $v{"if.$head[-$i]"} = 0 + $f[-$i];
    }
    return \%v;
}

sub link_row {
    my ($f) = @_;
    return @{$f} > 2 && index($f->[2], '<Link') == 0 ? 1 : 0;
}

# FreeBSD procstat -r lines: times in seconds, counts as integers
sub load_rusage {
    my ($text) = @_;
    my %v;
    for my $line (lines($text)) {
        next unless $line =~ /^\s*\d+\s+\S+\s+(.+?)\s{2,}(\S+)(?:\s+KB)?\s*$/;
        my ($name, $val) = (trim($1), $2);
        if ($val =~ /\A(\d+):(\d+):(\d+(?:\.\d+)?)\z/a) {
            $v{"rusage.$name s"} = $1 * 3600 + $2 * 60 + $3;
        } elsif (is_count($val)) {
            $v{"rusage.$name"} = 0 + $val;
        }
    }
    return \%v;
}

my @WATCH = (
             ['vfs.cache.stats.',         'name cache'],
             ['vfs.numvnodes',            'vnodes'],
             ['vfs.freevnodes',           'vnodes'],
             ['kern.maxvnodes',           'vnodes'],
             ['vfs.recycles',             'vnodes'],
             ['vfs.vnode_stats.',         'vnodes'],
             ['vfs.vnlru.',               'vnodes'],
             ['kstat.zfs.misc.arcstats.', 'ZFS ARC'],
             ['Tcp',                      'TCP, system-wide'],
             ['tcp ',                     'TCP, system-wide'],
             ['if.',                      'the interface'],
             ['vmstat.pg',                'memory'],
             ['vmstat.pswp',              'memory'],
             ['pressure.',                'pressure stalls'],
             ['pid.',                     'the traced afpd'],
             ['rusage.',                  'the traced afpd'],
);
my %LEVELS = map { $_ => 1 } qw(vfs.numvnodes vfs.freevnodes kern.maxvnodes kstat.zfs.misc.arcstats.size
  kstat.zfs.misc.arcstats.c kstat.zfs.misc.arcstats.c_max kstat.zfs.misc.arcstats.arc_meta_used
  kstat.zfs.misc.arcstats.metadata_size pid.status.VmRSS pid.status.Threads pid.fds), 'rusage.maximum RSS';
my @TCP_WORDS = (
                 'retrans', 'persist', 'window probe', 'zero window', 'duplicate ack', 'out-of-order',
                 'timeout', 'drop',    'Retrans',      'ZeroWindow',  'Timeouts',      'LossProbe',
                 'Drop',    'OFO',     'InErrs',       'OutSegs',     'InSegs',        'DelayedACK',
);

# changes over the run, grouped; levels show before and after
sub counter_report {
    my ($before, $after, $wall, $out) = @_;
    my @rows;
    for my $name (sort grep { exists $after->{$_} } keys %{$before}) {
        my $w = first { index($name, $_->[0]) == 0 } @WATCH or next;
        next if $w->[1] eq 'TCP, system-wide' && !grep { index($name, $_) >= 0 } @TCP_WORDS;
        my $delta = $after->{$name} - $before->{$name};
        if ($LEVELS{$name}) {
            push @rows, [$w->[1], $name, "$before->{$name} -> $after->{$name}"];
        } elsif ($delta) {
            # the only fractional counters are times, named "... s"
            push @rows, [$w->[1], $name, sprintf($name =~ / s\z/ ? '%+.3f' : '%+d', $delta)];
        }
    }
    my %done;
    for my $group (grep { !$done{$_}++ } map { $_->[1] } @WATCH) {
        my @lines = grep { $_->[0] eq $group } @rows;
        next unless @lines;
        push @{$out}, "== $group", map { sprintf('   %-52s %s', $_->[1], $_->[2]) } @lines;
    }

    # a process that ended has no figures after the run
    my $d = sub {
        my $n = 0;
        $n += exists $before->{$_} && exists $after->{$_} ? $after->{$_} - $before->{$_} : 0 for @_;
        return $n;
    };
    my ($hits, $neg, $miss) = map { $d->("vfs.cache.stats.$_") } qw(poshits neghits miss);
    if ($hits || $neg || $miss) {
        my $all = $hits + $neg + $miss;
        push @{$out},
          sprintf(
                  'name cache: %d hits, %d negative hits, %d misses, %.1f%% of lookups missed',
                  $hits, $neg, $miss, $all ? 100 * $miss / $all : 0
          );
    }
    my @arc = map { $d->("kstat.zfs.misc.arcstats.demand_$_") } qw(metadata_hits metadata_misses data_hits data_misses);
    if (grep {$_} @arc) {
        push @{$out},
          sprintf(
                   'ARC: metadata %d hits, %d misses; data %d hits, %d misses (each miss reads from a disk or an L2ARC '
                 . 'device)',
                 @arc
          );
    }
    my %disks = map { (split /\./)[1] => 1 } grep { /\Adisk\./ && /\.io_ms\z/ } keys %{$after};
    for my $disk (sort keys %disks) {

        # a partition's I/O counts in its disk's too
        if ($disk =~ /\A(.*\D)p?\d+\z/) {
            my $base = $1;
            (my $bare = $base) =~ s/p+\z//;
            next if $disks{$base} || $disks{$bare};
        }
        my $ios = $d->("disk.$disk.reads", "disk.$disk.writes") or next;
        push @{$out},
          sprintf(
                  'disk %s: %d I/Os, %.2f MiB, %.2f ms each, busy %.0f%% of the run',
                  $disk, $ios,
                  $d->("disk.$disk.read_sectors", "disk.$disk.write_sectors") * 512 / 1048576,
                  $d->("disk.$disk.read_ms",      "disk.$disk.write_ms") / $ios,
                  $wall ? 100 * $d->("disk.$disk.io_ms") / 1e3 / $wall : 0
          );
    }
    my $cpu_line =
      'the traced afpd: %.2f s on CPU (%.2f s user, %.2f s system), %d voluntary and %d involuntary context switches';
    if (my $ticks = $d->('pid.utime_ticks', 'pid.stime_ticks')) {
        my $hz = POSIX::sysconf(POSIX::_SC_CLK_TCK()) || 100;
        push @{$out},
          sprintf(
                  $cpu_line,
                  $ticks / $hz,
                  $d->('pid.utime_ticks') / $hz,
                  $d->('pid.stime_ticks') / $hz,
                  $d->('pid.status.voluntary_ctxt_switches'),
                  $d->('pid.status.nonvoluntary_ctxt_switches')
          );
    }
    if (my $cpu = $d->('rusage.user time s', 'rusage.system time s')) {
        push @{$out},
          sprintf(
                  $cpu_line,
                  $cpu,
                  $d->('rusage.user time s'),
                  $d->('rusage.system time s'),
                  $d->('rusage.voluntary context switches'),
                  $d->('rusage.involuntary context switches')
          );
    }
    return;
}

# FreeBSD iostat -x -w 1: per device, the mean and peak of each second's
# figures; the first block counts since boot and is skipped
sub iostat_report {
    my ($text, $out) = @_;
    my (@blocks, @head, $cur);
    for my $line (lines($text)) {
        my @f = split ' ', $line;
        next unless @f;
        if ($f[0] eq 'device') {
            @head = @f;
            push @blocks, $cur = {};
            next;
        }
        next unless @head && $cur && @f == @head && !grep { !looks_like_number($_) } @f[1 .. $#f];
        $cur->{$f[0]} = {map { $head[$_] => 0 + $f[$_] } 1 .. $#f};
    }
    shift @blocks;
    return unless @blocks;
    push @{$out}, sprintf('== disks, per second over %d s', scalar @blocks);
    my %devs = map {
        map { $_ => 1 }
          keys %{$_}
    } @blocks;
    for my $dev (sort keys %devs) {
        my @rows = map { $_->{$dev} } grep { $_->{$dev} } @blocks;
        my $mean = sub {
            my ($k) = @_;
            return sum(0, map { $_->{$k} // 0 } @rows) / @rows;
        };
        my @busy = map { $_->{'%b'} // 0 } @rows;
        next if !grep {$_} @busy and !grep { ($_->{'r/s'} // 0) + ($_->{'w/s'} // 0) } @rows;
        push @{$out},
          sprintf(
                  '   %-10s busy mean %.0f%% peak %.0f%%; r/s %.0f w/s %.0f; ms per transaction mean %.1f peak %.1f',
                  $dev, $mean->('%b'), max(@busy), $mean->('r/s'), $mean->('w/s'), $mean->('ms/t'),
                  max(map { $_->{'ms/t'} // 0 } @rows)
          );
    }
    return;
}

# ---- traces ----

# the sum of the numbers re captures; re carries its own /m
sub map_sum {
    my ($text, $re) = @_;
    my $n = 0;
    $n += $1 while $text =~ /$re/g;
    return $n;
}

# totals of a request trace: time on CPU, asleep and runnable, the commonest
# reasons for sleeps, and the CNID calls. The summary a trace prints at its end
# counts every request; its REQ lines stand in when the summary is missing, and
# a trace that lost events says so.
sub trace_totals {
    my ($text, $out, $kind) = @_;
    $text //= '';
    my @lines = map { [split ' ', $_] } lines($text);

    # nanoseconds: requests, in afpd, on CPU, asleep, runnable
    my @tot = (0) x 5;
    for my $f (@lines) {
        next unless @{$f};
        if ($f->[0] eq 'CMD' && @{$f} == 4) {
            $tot[$_ - 2] += $f->[$_] for 2, 3;
        } elsif ($f->[0] eq 'TIME' && @{$f} == 5) {
            $tot[$_] += $f->[$_] for 2 .. 4;
        }
    }
    my %at = (n => 0, ns => 1, oncpu => 2, asleep => 3, runnable => 4);
    $tot[$at{$1}] += $2 while $text =~ /^\@(n|ns|oncpu|asleep|runnable)\[[^\]]*\]: (\d+)$/mg;
    if (!$tot[0]) {
        my @reqs = grep { @{$_} == 7 && $_->[0] eq 'REQ' } @lines;
        return unless @reqs;
        my $i;
        @tot = (
                scalar @reqs,
                map {
                    $i = $_;
                    1000 * sum(0, map { $_->[$i] } @reqs)
                } 3 .. 6
        );
    }

    # the probes bracket the AFP command's handler; reading the request and
    # sending the reply lie outside, and the capture's server time has both
    push @{$out},
      sprintf(
              '== %s trace of the traced afpd: %d requests, %.3f s in their AFP handlers', $kind, $tot[0],
              $tot[1] / 1e9
      ),
      sprintf('   on CPU %.3f s, asleep %.3f s, runnable %.3f s', $tot[2] / 1e9, $tot[3] / 1e9, $tot[4] / 1e9);
    if (my $lost = map_sum($text, qr/^Lost (\d+) events/m)) {
        push @{$out}, "   the trace lost $lost events: its REQ lines miss some requests; the totals do not";
    }
    my $reasons = counter();
    bump($reasons, $_->[3], $_->[5]) for grep { @{$_} >= 7 && $_->[0] eq 'OFF' } @lines;
    bump($reasons, $1,      $2) while $text =~ /^\@offs\[[^,\]]*, ([^\]]*)\]: (\d+)$/mg;
    for my $why (grep { $reasons->{n}{$_} } most_common($reasons, 3)) {
        push @{$out}, sprintf('   asleep on %s: %.3f s', $why, $reasons->{n}{$why} / 1e9);
    }
    my @cnid  = grep { @{$_} >= 5 && $_->[0] eq 'CNID' } @lines;
    my $calls = sum(0, map { $_->[3] } @cnid) + map_sum($text, qr/^\@cnid\[[^\]]*\]: (\d+)$/m);
    my $ns    = sum(0, map { $_->[4] } @cnid) + map_sum($text, qr/^\@cnidns\[[^\]]*\]: (\d+)$/m);
    push @{$out}, sprintf('   CNID calls: %d, %.3f s', $calls, $ns / 1e9) if $calls;
    return;
}

# the system calls of a -s trace: their count and time, and the calls that took
# the most
sub syscall_totals {
    my ($text, $out, $kind) = @_;
    $text //= '';
    my ($calls, $ns) = (counter(), counter());
    for my $line (lines($text)) {
        my @f = split ' ', $line;
        next unless @f == 4 && $f[0] eq 'SYS';
        bump($calls, $f[1], $f[2]);
        bump($ns,    $f[1], $f[3]);
    }
    while ($text =~ /^\@(n|ns)\[tracepoint:syscalls:sys_exit_(\w+)\]: (\d+)$/mg) {
        bump($1 eq 'n' ? $calls : $ns, $2, $3);
    }
    return unless @{$calls->{order}};
    push @{$out},
      sprintf(
              '== %s system calls of the traced afpd: %d calls, %.3f s',
              $kind, sum(0, values %{$calls->{n}}), sum(0, values %{$ns->{n}}) / 1e9
      );
    for my $name (most_common($ns, 8)) {
        push @{$out}, sprintf('   %-24s %9d calls %9.3f s', $name, $calls->{n}{$name} // 0, $ns->{n}{$name} / 1e9);
    }
    return;
}

my %STATE_NAMES = (
                   R => 'running or runnable',
                   S => 'asleep',
                   D => 'in uninterruptible sleep, mostly disk',
                   Z => 'exited',
                   T => 'stopped',
);

# the Linux sampler's counts: the share of samples in each state, and the
# commonest wait channels of the sleeping ones
sub samples_report {
    my ($text, $out) = @_;
    my @rows = map { [@{$_}[1, 2], 0 + $_->[3]] }
      grep { @{$_} == 4 && $_->[0] eq 'SAMPLE' && is_count($_->[3]) } map { [split ' ', $_] } lines($text);
    my $total = sum(0, map { $_->[2] } @rows) or return;
    push @{$out}, "== samples of the traced afpd's threads: $total";
    my $states = counter();
    bump($states, $_->[0], $_->[2]) for @rows;
    for my $state (most_common($states)) {
        push @{$out}, sprintf('   %-40s %5.1f%%', $STATE_NAMES{$state} // $state, 100 * $states->{n}{$state} / $total);
    }
    my @top = sort { $b->[2] <=> $a->[2] } @rows;
    splice(@top, 8) if @top > 8;
    push @{$out},
      map { sprintf('   %s on %-30s %5.1f%%', @{$_}[0, 1], 100 * $_->[2] / $total) } grep { $_->[0] ne 'R' } @top;
    return;
}

# ---- the log of -m ----

my $TS       = qr/^(\w{3}) +(\d+) (\d\d):(\d\d):(\d\d)\.(\d{6}) /a;
my $START    = qr/<== Start AFP command: (\S+)/;
my $DONE     = qr/==> Finished AFP command: (\S+) -> (\S+)/;
my $LOG_MARK = qr/\Q$MARK\E([A-Za-z0-9_]+)/;
my @EVENTS = (
              [miss         => qr/\{not in cache\}/],
              [hit          => qr/\{found in cache\}|\{cached \(unvalidated\)/],
              [validated    => qr/\{validated/],
              [cnid_query   => qr/querying CNID database/],
              [walk         => qr/movecwd\(to: did/],
              [selfheal     => qr/movecwd: stale dircache entry|self-healing/],
              [added        => qr/dir_add_fill\(.*\{cached:/],
              [removed      => qr/\{removed\}/],
              [listing_read => qr/enumerate: cache miss/],
              [ad_read      => qr/ad_metadata\(|ad_open\(.*BEGIN/],
);
my $STATS = qr/(dircache statistics[^:]*:|pfd_cache:|enumerate statistics:|ARC operations:|ARC ghost performance:)(.*)/;
my %MONTH = map { (qw(Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec))[$_] => $_ } 0 .. 11;
my @MDAYS = (31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31);
my $YEAR_US = 366 * 86400 * 1e6;

# the stamp in microseconds, with its day, in a leap year the log does not
# name; a request that spans the year's end reads YEAR_US back
sub log_usec {
    my ($mon, $day, $h, $mi, $s, $us) = @_;
    my $m    = $MONTH{$mon} // return undef;
    my $days = sum(0, @MDAYS[0 .. $m - 1]) + $day - 1;
    return ((($days * 24 + $h) * 60 + $mi) * 60 + $s) * 1e6 + $us;
}

sub events_text {
    my ($ev) = @_;
    return join(', ', map {"$_ $ev->{n}{$_}"} @{$ev->{order}}) || '-';
}

# per phase of afpd maxdebug logs: request durations and dircache events.
# maxdebug slows afpd: compare phases of one log only.
sub log_report {
    my ($paths, $out) = @_;
    my (@reqs, @stats);
    for my $path (@{$paths}) {
        my $cur;

        # names such as "Icon\r" hold a CR, so only "\n" ends a line
        for my $line (split /\n/, read_text($path) // '') {
            my $t = $line =~ $TS ? log_usec($1, $2, $3, $4, $5, $6) : undef;
            if (defined $t && $line =~ $START) {
                $cur = {cmd => $1, t0 => $t, ev => counter(), mark => undef};
                next;
            }
            if (defined $cur && defined $t && $line =~ $DONE) {
                $cur->{us}     = $t - $cur->{t0} + ($t < $cur->{t0} ? $YEAR_US : 0);
                $cur->{result} = $2;
                push @reqs, $cur;
                undef $cur;
                next;
            }
            # afpd names the session's user in its statistics
            push @stats, trim("$1$2") =~ s/\(user: [^)]*\) *//r if $line =~ $STATS;
            next unless $cur;
            $cur->{mark} = $1 if $line =~ $LOG_MARK;
            for my $e (@EVENTS) {
                bump($cur->{ev}, $e->[0]) if $line =~ $e->[1];
            }
        }
    }
    my ($phase, @tagged) = ('all');
    for my $r (@reqs) {
        if (defined $r->{mark}) {
            $phase = $r->{mark};
            next;
        }
        $r->{phase} = $phase;
        push @tagged, $r;
    }
    for my $g (group_by(sub { $_[0]{phase} }, @tagged)) {
        my ($name, $rs) = @{$g};
        my $ev = counter();
        for my $r (@{$rs}) {
            bump($ev, $_, $r->{ev}{n}{$_}) for @{$r->{ev}{order}};
        }
        push @{$out},
          sprintf(
                  '== phase %s: %d requests, %.1f ms in afpd', $name, scalar @{$rs},
                  sum(0, map { $_->{us} } @{$rs}) / 1000
          ),
          sprintf('   %-22s %6s %10s %9s %9s', qw(command n total_ms mean_us max_us));
        my @by = map {
            [$_->[0], [map { $_->{us} } @{$_->[1]}]]
        } group_by(sub { $_[0]{cmd} }, @{$rs});
        for my $c (sort { sum(@{$b->[1]}) <=> sum(@{$a->[1]}) } @by) {
            my ($cmd, $us) = @{$c};
            push @{$out},
              sprintf('   %-22s %6d %10.1f %9.0f %9d', $cmd, scalar @{$us}, sum(@{$us}) / 1000, sum(@{$us}) / @{$us},
                      max(@{$us}));
        }
        push @{$out}, '   events: ' . join(', ', map {"$_->[0] $ev->{n}{$_->[0]}"} grep { $ev->{n}{$_->[0]} } @EVENTS);
        my @slow = sort { $b->{us} <=> $a->{us} } @{$rs};
        splice(@slow, 5) if @slow > 5;
        push @{$out},
          map { sprintf('   slow: %8d us %-20s %-14s %s', $_->{us}, $_->{cmd}, $_->{result}, events_text($_->{ev})) }
          @slow;
    }
    push @{$out}, '== session statistics', map {"   $_"} @stats if @stats;
    return;
}

# ---- the configuration ----

# the settings that shape how requests are served keep their values; every
# other value names a host, a path, a user, a group or a message
my %KEEP_KEYS = map { $_ => 1 } (
                                 'follow symlinks',     'case sensitive',    'casefold',       'ea',
                                 'cnid scheme',         'cnid dev',          'read only',      'unix charset',
                                 'vol charset',         'mac charset',       'log level',      'log microseconds',
                                 'vol size limit',      'stat vol',          'spotlight',      'search db',
                                 'convert appledouble', 'delete veto files', 'invisible dots', 'legacy icon',
                                 'time machine',        'acls',              'map acls',       'chmod request',
                                 'unix priv',           'strict locking',    'file perm',      'directory perm',
                                 'umask',               'server quantum',    'dsireadbuf',     'tcprcvbuf',
                                 'tcpsndbuf',           'use sendfile',      'splice size',    'recvfile',
                                 'preexec close',       'zeroconf',          'uam list',       'multi protocol',
                                 'sleep time',          'disconnect time',   'tickleval',      'timeout',
                                 'max connections',     'spotlight backend',
);

sub kept_key {
    my ($key) = @_;
    return $KEEP_KEYS{$key} || index($key, 'dircache') == 0;
}

# the sections of an afp.conf, each {name, vol, title, opts}: name is global,
# homes or volume, vol the volume's number from 1, and opts [key, value, line]
sub parse_conf {
    my ($text) = @_;
    my (@sections, $cur);
    my $n = 0;
    for my $line (lines($text)) {
        my $s = trim($line);
        next if $s eq '' || $s =~ /\A[;#]/;
        if (substr($s, 0, 1) eq '[') {
            my $low = lc $s;
            $cur = {opts => [], title => $s =~ /\A\[([^\]]*)\]/ ? $1 : substr($s, 1)};
            if (index($low, '[global]') == 0) {
                $cur->{name} = 'global';
            } elsif (index($low, '[homes]') == 0) {
                $cur->{name} = 'homes';
            } else {
                @{$cur}{qw(name vol)} = ('volume', ++$n);
            }
            push @sections, $cur;
            next;
        }
        next unless $cur && $line =~ /^\s*([^;#=\s][^=]*?)\s*=\s*(.*)$/;
        my ($key, $value) = (lc trim($1), $2);
        $value =~ s/\s*[;#].*$//;
        $value =~ s/[\\ \t]+\z//;
        push @{$cur->{opts}}, [$key, trim($value), $line];
    }
    return \@sections;
}

sub redact_conf {
    my ($sections) = @_;
    my @out;
    for my $sec (@{$sections}) {
        my %title = (global => 'Global', homes => 'Homes');
        push @out, '[' . ($title{$sec->{name}} // "volume $sec->{vol}") . ']';
        push @out, map { kept_key($_->[0]) ? " $_->[0] = $_->[1]" : " $_->[0] = REDACTED" } @{$sec->{opts}};
        push @out, '';
    }
    return @out;
}

sub as_int {
    my ($value) = @_;
    return defined $value && $value =~ /\A\s*(\d+)/a ? 0 + $1 : undef;
}

# one line for each setting whose cost is known
sub config_notes {
    my ($sections, $out) = @_;
    my (%g, @vols);
    for my $sec (@{$sections}) {
        my %d = map { $_->[0] => $_->[1] } @{$sec->{opts}};
        if ($sec->{name} eq 'global') {
            %g = %d;
        } else {
            push @vols, \%d;
        }
    }
    my $any_vol = sub {
        my ($key, $test) = @_;
        return $test->(lc($g{$key} // '')) || grep { $test->(lc($_->{$key} // '')) } @vols;
    };
    my $level = $g{'log level'} // '';
    push @{$out}, "log level = $level: debug logging adds work to every request; the default is default:note"
      if $level =~ /:\s*(debug\d*|maxdebug)\b/i;
    push @{$out},
      'cnid scheme = mysql: every CNID lookup is a round trip to the MySQL server; sqlite, the default, answers in afpd'
      if $any_vol->('cnid scheme', sub { $_[0] eq 'mysql' });
    for my $key (qw(tcpsndbuf tcprcvbuf)) {
        next if ($g{$key} // '') eq '';
        push @{$out},
          "$key = $g{$key}: a fixed TCP buffer size turns off the kernel's automatic sizing for every connection";
    }
    my $size = as_int($g{'dircache size'});
    push @{$out}, "dircache size = $size, below the default 65536: folders past it are looked up again"
      if defined $size && $size < 65536;
    my $freq = as_int($g{'dircache validation freq'});
    push @{$out},
      "dircache validation freq = $freq: afpd checks cached entries against the disk more often than the default 100"
      if defined $freq && $freq < 100;
    push @{$out},
      'multi protocol = yes: afpd checks every cached entry against the disk and caches no resource forks for the '
      . 'volume'
      if $any_vol->('multi protocol', sub { $_[0] eq 'yes' || $_[0] eq 'true' });
    push @{$out}, 'use sendfile = no: reads copy file data through afpd instead of the kernel sending it'
      if lc($g{'use sendfile'} // '') =~ /\A(no|false)\z/;
    my $quantum = as_int($g{'server quantum'});
    push @{$out}, "server quantum = $quantum, below the default 1048576: more round trips per MiB"
      if defined $quantum && $quantum < 1048576;
    push @{$out}, 'dircache rfork budget = 0: no resource fork cache, so every read of a custom icon goes to the disk'
      if (as_int($g{'dircache rfork budget'}) // -1) == 0;
    return;
}

# [number, path] of each volume with a path; [Homes] has none of its own
sub volume_paths {
    my ($sections) = @_;
    my @paths;
    for my $sec (grep { defined $_->{vol} } @{$sections}) {
        push @paths, map { [$sec->{vol}, $_->[1]] } grep { $_->[0] eq 'path' && $_->[1] ne '' } @{$sec->{opts}};
    }
    return @paths;
}

sub unescape_octal {
    my ($s) = @_;
    $s =~ s/\\([0-7]{3})/chr oct $1/ge;
    return $s;
}

# [mount point, filesystem type, source, options] from Linux mountinfo or
# FreeBSD 'mount -p'
sub read_mounts {
    my ($text) = @_;
    my @mounts;
    for my $line (lines($text)) {
        my @f = split ' ', $line;
        if (index($line, ' - ') >= 0 && @f >= 10) {

            # the mount's own options, then the filesystem's
            my $sep = first { $f[$_] eq '-' } 0 .. $#f;
            next unless defined $sep;
            my $opts = $f[5] . (@f > $sep + 3 ? ",$f[$sep + 3]" : '');
            push @mounts, [unescape_octal($f[4]), $f[$sep + 1], unescape_octal($f[$sep + 2]), $opts];
        } elsif (@f >= 4) {
            push @mounts, [unescape_octal($f[1]), $f[2], unescape_octal($f[0]), $f[3]];
        }
    }
    return \@mounts;
}

# the path with the folders that exist resolved, the rest as written
sub real_path {
    my ($path) = @_;
    my $real = Cwd::realpath($path);
    return $real if defined $real;
    my ($head, @tail) = ($path);
    while ($head =~ m{\A(.*?)/+([^/]+)/*\z}s) {
        ($head, @tail) = ($1, $2, @tail);
        my $h = Cwd::realpath($head eq '' ? '/' : $head);
        return join('/', $h eq '/' ? '' : $h, @tail) if defined $h;
    }
    return $path;
}

sub mount_of {
    my ($path, $mounts) = @_;
    my $real = real_path($path);
    my ($best, $best_len);
    for my $m (@{$mounts}) {
        (my $mp = $m->[0]) =~ s{/+\z}{};
        $mp = '/' if $mp eq '';
        next unless $real eq $mp || index($real, "$mp/") == 0 || $mp eq '/';
        ($best, $best_len) = ($m, length $mp) if !$best || length $mp > $best_len;
    }
    return $best;
}

my %NETWORK_FS =
  map { $_ => 1 } qw(nfs nfs4 cifs smbfs smb3 afpfs 9p ceph glusterfs davfs sshfs fuse.sshfs fusefs.sshfs);
my %OPTION_VALUES = map { $_ => 1 } qw(vers rsize wsize proto timeo retrans nconnect sec actimeo cache readahead);

sub remote_fs {
    my ($type) = @_;
    return $NETWORK_FS{$type} || index($type, 'fuse') == 0;
}

sub volume_lines {
    my ($sections, $mounts) = @_;
    my @out;
    for my $vp (volume_paths($sections)) {
        my ($n, $path) = @{$vp};
        my $m = mount_of($path, $mounts);
        if (!$m) {
            push @out, "volume $n: no mount found";
            next;
        }
        my @opts = map {
            my ($k, $v) = split /=/, $_, 2;
            !defined $v || ($OPTION_VALUES{$k} && $v =~ /\A[\w.]{1,16}\z/) ? $_ : $k
        } split /,/, $m->[3];
        my $kind =
            $NETWORK_FS{$m->[1]}        ? 'a network filesystem: each request also waits on another server'
          : index($m->[1], 'fuse') == 0 ? 'a FUSE filesystem: each request also crosses into a user-space filesystem'
          :                               'local';
        push @out, "volume $n: $m->[1], options " . join(',', @opts) . "; $kind";
    }
    return @out;
}

# ---- what to share ----

# paths keep their depth only: every component reads '+'
sub depth {
    my ($text) = @_;
    return join("\n", map {s{/[^/]*}{/+}gr} split(/\n/, $text, -1));
}

my $V4  = qr/\b[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+(:[0-9]+)?/;
my $V6  = qr/\[?([0-9a-fA-F]{0,4}:){2,7}[0-9a-fA-F]{0,4}\]?(:[0-9]+)?/;
my $MAC = qr/\b([0-9a-fA-F]{2}[:-]){5}[0-9a-fA-F]{2}\b/;

# addresses read ADDR and hardware addresses MAC
sub addr {
    my ($text) = @_;
    $text =~ s/$MAC/MAC/g;
    $text =~ s/$V4/ADDR/g;
    $text =~ s/$V6/ADDR/g;
    return $text;
}

# the dump's ghost lists name an entry without its path
sub ghosts {
    my ($text) = @_;
    $text =~ s/^([0-9]+: +[0-9]+ +[0-9]+ +[0-9]+ [fd] +).* \(GHOST\)$/$1+ (GHOST)/mg;
    return $text;
}

# path components and words too common to name anyone, which the reports' own
# text uses too
my @GENERIC = qw(
    afp afpd netatalk srv data home homes mnt media var tmp usr local opt etc share shares export exports storage
    volume volumes vol users user time machine backup backups tank zroot pool zfs public private guest test nas server
    files documents desktop downloads music pictures movies library applications system root admin nobody icon default
    global network bin sbin lib lib64 libexec include man log logs run spool cache db mail boot dev proc sys
);

# the strings a shared file must not hold: names, paths, users, hosts and
# addresses this run saw in the raw data. A word counts whole, a phrase by
# its longest word first, and an address anywhere.
sub new_sensitive {
    return {words => {}, phrases => {}, by_key => {}, literals => [], generic => {map { $_ => 1 } @GENERIC}};
}

# words the shared files hold by design, such as afp.conf's option names,
# which no string watched for can be
sub sens_allow {
    my ($s, @words) = @_;
    $s->{generic}{lc $_} = 1 for @words;
    return;
}

# an address, which must not appear anywhere, a port after it or not
sub sens_add_literal {
    my ($s, $lit) = @_;
    $lit = trim($lit // '');
    return if length $lit < 3 || grep { $_->[0] eq $lit } @{$s->{literals}};
    push @{$s->{literals}}, [$lit, qr/(?<![0-9A-Za-z.:])\Q$lit\E(?![0-9A-Za-z])/];
    return;
}

sub sens_add {
    my ($s, $str) = @_;
    $str = trim($str // '');
    return
         if length $str < 3
      || $str =~ /\A\d+\z/
      || $s->{generic}{lc $str}
      || index($str, $MARK) == 0
      || $KEEP_NAMES{$str};
    if ($str =~ /\A\w+\z/) {
        $s->{words}{$str} = 1;
        return;
    }
    my @keys = grep { length($_) >= 3 } $str =~ /(\w+)/g;
    return if !@keys || $s->{phrases}{$str};

    # a line can hold the phrase only if it holds its longest word
    my $key = $keys[0];
    for my $k (@keys) {
        $key = $k if length($k) > length($key);
    }
    $s->{phrases}{$str} = qr/(?<!\w)\Q$str\E(?!\w)/;
    push @{$s->{by_key}{$key}}, $str;
    return;
}

sub sens_add_path {
    my ($s, $path) = @_;
    sens_add($s, $_) for $path, split m{/}, $path;
    return;
}

sub sens_scan {
    my ($s, $line) = @_;
    my %words = map  { $_ => 1 } $line =~ /(\w+)/g;
    my @hits  = grep { $s->{words}{$_} } sort keys %words;
    for my $word (sort keys %words) {
        push @hits, grep { $line =~ $s->{phrases}{$_} } @{$s->{by_key}{$word} || []};
    }
    push @hits, map { $_->[0] } grep { index($line, $_->[0]) >= 0 && $line =~ $_->[1] } @{$s->{literals}};
    return @hits;
}

# files whose lines hold kernel counter and disk device names and numbers
my $COUNTER_FILES = qr/\A(counters-|vmstat-z-|report-counters)/;

sub leak_scan {
    my ($share, $s) = @_;
    my @hits;
    for my $name (list_dir($share)) {
        next if $name eq 'README.txt' || $name =~ $COUNTER_FILES;
        my $no = 0;
        for my $line (split /\n/, read_text("$share/$name") // '', -1) {
            $no++;
            push @hits, map { [$name, $no, $_] } sens_scan($s, $line);
        }
    }
    return @hits;
}

my %ZFS_REDACTED = map { $_ => 1 }
  qw(mountpoint origin sharenfs sharesmb encryptionroot keylocation receive_resume_token redact_snaps mlslabel guid);

# zfs get output with dataset and pool names as roles; a property that names a
# place, a snapshot, a share or a key, or tells the dataset apart, loses its
# value; user properties go
sub zfs_redact {
    my ($text, $names) = @_;
    my @out;
    for my $line (lines($text)) {
        my @f = split ' ', $line;
        next if @f >= 2 && index($f[1], ':') >= 0;
        @f = @f[0 .. 3]                    if @f >= 5 && $f[3] eq 'inherited';
        @f = (@f[0, 1], 'REDACTED', $f[3]) if @f >= 4 && $ZFS_REDACTED{$f[1]};
        push @out, roles(join(' ', @f), $names);
    }
    return join("\n", @out) . "\n";
}

my $VDEV =
qr/^(mirror|raidz[123]?|draid[123]?|spare|replacing|logs|cache|spares|special|dedup)(-\d+)?$|^(NAME|pool:|state:|scan:|config:|errors:|status:|action:|see:|scrub|capacity|operations|bandwidth|-+)/;

# zpool status or iostat with the pool as its role, each device as DEVICE and
# each entry of the errors list as ENTRY
sub zpool_redact {
    my ($text, $names) = @_;
    my %is_name = map { $_->[0] => 1 } @{$names};
    my (@out, $errors);
    for my $line (lines($text)) {
        if (index($line, 'errors:') == 0) {
            $errors = 1;
        } elsif ($errors && $line =~ /\A\s/ && $line =~ /\S/) {
            push @out, '        ENTRY';
            next;
        }
        my @f = split ' ', $line;
        if (@f && $line =~ /\A\s/ && $f[0] !~ $VDEV && !$is_name{$f[0]}) {
            substr($line, index($line, $f[0]), length $f[0]) = 'DEVICE';
        }
        push @out, roles($line, $names);
    }
    return join("\n", @out) . "\n";
}

# names: [name, role] pairs; the longest name first, so a dataset is not read
# as its pool
sub roles {
    my ($line, $names) = @_;
    for my $p (sort { length $b->[0] <=> length $a->[0] } @{$names}) {
        my ($name, $role) = @{$p};
        $line =~ s/(?<![\w.-])\Q$name\E(?![\w.-])/$role/g;
    }
    return $line;
}

my $README = <<'EOF';
netatalk-diag report, %s

No file, folder, volume, user or host name and no address is in these
files: netatalk-diag checked every file against the names, paths, users,
hosts and addresses it saw before it packaged them. A path keeps its depth
as /+/+, the request reports name files and folders by per-run tokens,
afp.conf keeps its option names with identifying values removed, users and
clients are numbered, and ZFS datasets and pools read DATASETn and POOLn.

summary.txt         where the time went, TCP health, transfers, logins,
                    configuration notes and what was not collected
report-pcap.txt     the AFP requests per phase and command
requests.csv        every request, with its times, sizes and fork
report-counters.txt kernel and afpd counter changes over the run
report-trace.txt    what the traced afpd did per request (-t)
report-log.txt      the afpd maxdebug log per phase (-m)
dircache.*          the session's dircache before and after (-m)
dtrace-*, bpftrace-*  the raw trace output (-t)
samples.txt         the traced afpd's thread states (Linux)
baseline.txt        the read-only walk of the volumes (-b)
volumes.txt         each volume's filesystem
network.txt         the interface and the TCP settings
ss-*                the AFP connections' TCP state before and after (Linux)
sessions.txt        the afpd sessions seen, numbered
system.txt, afpd-V.txt, afp.conf, zfs-*, zpool-*   the configuration
counters-*, vmstat-z-*, ps-*, procstat-*  snapshots before and after
EOF

my %AS_IS = map { $_ => 1 } qw(vmstat-z-before.txt vmstat-z-after.txt system.txt tcpdump.txt dtrace-requests.txt
  dtrace-syscalls.txt dtrace-zfs.txt bpftrace-requests.txt bpftrace-syscalls.txt samples.txt baseline.txt iostat.txt);
my %ADDRESSED = map { $_ => 1 } qw(network.txt ss-before.txt ss-after.txt);

# what identity.txt, afp.conf and the mounts name, for the leak check
sub watch_identity {
    my ($s, $raw, $sections, $mounts) = @_;
    for my $line (lines($raw->('identity.txt'))) {
        my ($kind, $value) = split / /, $line, 2;
        next unless defined $value;
        if ($kind eq 'address' || $kind eq 'mac' || $kind eq 'client') {
            sens_add_literal($s, $value);
        } elsif ($kind eq 'host' || $kind eq 'user') {
            sens_add($s, $_) for $value, (split /\./, $value)[0];
        }
    }
    sens_allow($s, map { $_->[0] =~ /(\w+)/g } map { @{$_->{opts}} } @{$sections});
    for my $sec (@{$sections}) {
        sens_add($s, $sec->{title}) if defined $sec->{vol};
        sens_add_path($s, $_->[1]) for grep { !kept_key($_->[0]) } @{$sec->{opts}};
    }
    for my $vp (volume_paths($sections)) {
        my $m = mount_of($vp->[1], $mounts) or next;
        sens_add_path($s, $m->[0]);

        # a local source is a device, which names nobody; a remote one names
        # its server and export
        sens_add_path($s, $m->[2] =~ s{[:@]}{/}gr) if remote_fs($m->[1]);
    }
    return;
}

# summary lines from the capture, with requests.csv and report-pcap.txt
sub capture_report {
    my ($path, $share, $tokens, $write, $summary) = @_;
    my ($conns, $rows) = ([], []);
    if (!eval { ($conns, $rows) = analyse_capture($path, 548, $tokens); 1 }) {
        die $@ unless $@ =~ /\Anot a classic pcap file/;
        push @{$summary}, 'capture: ' . trim($@);
    }
    write_csv($rows, "$share/requests.csv");
    my @lines;
    report_requests($rows, \@lines);
    $write->('report-pcap.txt', join("\n", @lines));
    push @{$summary}, 'Where the time went, from the capture';
    push @{$summary}, '   no AFP requests in the capture' unless @{$rows};
    verdicts($rows, $summary);
    tcp_health($conns, $summary);
    transfers($rows, $summary);
    my @lost = map { $_->{label} } grep { $_->{c2s}{gaps} || $_->{s2c}{gaps} } @{$conns};
    push @{$summary}, '   the capture lost bytes of ' . join(', ', @lost) . ': frames after each loss are skipped'
      if @lost;
    my @log_in;
    logins($conns, $rows, \@log_in);
    push @{$summary}, '== logins', @log_in if @log_in;
    return;
}

# the kernel's and afpd's counters before and after, by their names
sub load_counters {
    my ($raw, $tag) = @_;
    my %v;
    %v = (%v, %{load_sysctl($raw->("counters-$tag.txt"))});
    %v = (%v, %{load_linux($raw->("proc-$tag.txt"))});
    %v = (%v, %{load_netstat_s($raw->("netstat-tcp-$tag.txt"))});
    %v = (%v, %{load_rusage($raw->("rusage-$tag.txt"))});
    %v = (%v, %{load_table($raw->("netstat-if-$tag.txt"), trim($raw->('iface.txt') // ''))});
    return \%v;
}

# write FOLDER, which must not exist, from what netatalk-diag collected in
# OUTDIR: 0, or 3 when a shared file holds a name or address the run saw, and
# FOLDER then stays for review and nothing is packaged
sub report {
    my ($outdir, $share, $force) = @_;
    mkdir $share or die "netatalk-diag: $share: $!\n";
    my $raw   = sub { read_text("$outdir/$_[0]") };
    my $write = sub {
        my ($name, $text) = @_;
        write_text("$share/$name", $text =~ /\n\z/ ? $text : "$text\n");
    };
    my $sensitive = new_sensitive();
    my ($users, $clients) = (new_labels('user'), new_labels('client'));
    my $sections = parse_conf($raw->('afp.conf.raw'));
    my $mounts   = read_mounts($raw->('mounts.txt'));
    watch_identity($sensitive, $raw, $sections, $mounts);
    $write->('afp.conf', join("\n", redact_conf($sections))) if @{$sections};
    my @vols = volume_lines($sections, $mounts);
    $write->('volumes.txt', join("\n", @vols)) if @vols;

    # sessions.txt: pid user client traced
    my @sess = map {
        sprintf('session pid %s, %s, %s%s', $_->[0], label($users, $_->[1]), label($clients, $_->[2]),
                $_->[3] eq '1' ? ', traced' : '')
    } grep { @{$_} == 4 } map { [split ' ', $_] } lines($raw->('sessions.txt'));
    $write->('sessions.txt', join("\n", @sess)) if @sess;
    for my $name ('ps-before.txt', 'ps-after.txt') {
        my $text = $raw->($name) // next;
        my @lines;
        for my $line (lines($text)) {
            my @f = split ' ', $line;

            # a user who logged in during the run shows here alone
            if (@f >= 3 && is_count($f[0])) {
                sens_add($sensitive, $f[2]);
                $f[2] = label($users, $f[2]);
            }
            push @lines, join(' ', @f);
        }
        $write->($name, join("\n", @lines));
    }

    # the datasets: n dataset
    my (@roles, %role_at);
    for my $line (lines($raw->('datasets.txt'))) {
        my ($n, $ds) = split / /, $line, 2;
        next unless defined $ds && $ds ne '';
        for my $r ([$ds, "DATASET$n", 1], [(split m{/}, $ds)[0], "POOL$n", 0]) {
            my ($name, $role, $replace) = @{$r};
            if (!exists $role_at{$name}) {
                push @roles, [$name, $role];
                $role_at{$name} = $#roles;
            } elsif ($replace) {
                $roles[$role_at{$name}][1] = $role;
            }
        }
        sens_add_path($sensitive, $ds);
    }
    for my $name (grep {/\A(zfs|zpool)-/} list_dir($outdir)) {
        my $text = $raw->($name) // next;
        $write->($name, index($name, 'zfs-') == 0 ? zfs_redact($text, \@roles) : zpool_redact($text, \@roles));
    }

    my @summary;
    my $tokens = new_names(0);
    capture_report("$outdir/afp.pcap", $share, $tokens, $write, \@summary) if -e "$outdir/afp.pcap";
    sens_add_path($sensitive, $_) for keys %{$tokens->{seen}};

    my @trace;
    trace_totals($raw->("$_->[1]-requests.txt"), \@trace, $_->[0])   for ['DTrace', 'dtrace'], ['bpftrace', 'bpftrace'];
    syscall_totals($raw->("$_->[1]-syscalls.txt"), \@trace, $_->[0]) for ['DTrace', 'dtrace'], ['bpftrace', 'bpftrace'];
    samples_report($raw->('samples.txt'), \@trace);
    if (@trace) {
        $write->('report-trace.txt', join("\n", @trace));
        push @summary, @trace;
    }

    my @stamps = split ' ', $raw->('times.txt') // '';
    my $wall =
      @stamps >= 2 && looks_like_number($stamps[0]) && looks_like_number($stamps[1]) ? $stamps[1] - $stamps[0] : 0;
    my ($before, $after) = (load_counters($raw, 'before'), load_counters($raw, 'after'));
    my @counters;
    counter_report($before, $after, $wall, \@counters);
    iostat_report($raw->('iostat.txt'), \@counters);
    if (@counters) {
        $write->('report-counters.txt', join("\n", @counters));
        push @summary, '== counters', grep { !/\A(== |   )/ } @counters;
    }

    # every counter, by its kernel or proc name, and a number
    for my $c (['counters-before.txt', $before], ['counters-after.txt', $after]) {
        my ($name, $v) = @{$c};
        $write->($name, join("\n", map {"$_: $v->{$_}"} sort keys %{$v})) if %{$v};
    }

    my @notes;
    config_notes($sections, \@notes);
    push @summary, '== configuration notes', map {"   $_"} @notes if @notes;
    my $collected = trim($raw->('notes.txt') // '');
    push @summary, '== collection notes', map {"   $_"} lines($collected) if $collected ne '';
    $write->('summary.txt', @summary ? join("\n", @summary) : 'nothing to report');

    my @logs = map {"$outdir/$_"} grep {/\Aafpd\.\d+\./} list_dir($outdir);
    if (@logs) {
        my @lines;
        log_report(\@logs, \@lines);
        $write->('report-log.txt', join("\n", @lines));
    }
    my $prefix = dec($outdir) =~ s{/*\z}{/}r;
    for my $name (list_dir($outdir)) {
        my $text;
        if ($name =~ /\Adircache\.\d+\.(first|second)\z/) {
            $text = ghosts(depth($raw->($name) // ''));
        } elsif ($name =~ /\A(procstat-files-|dtrace-.*\.err|bpftrace-.*\.err)/) {

            # a trace's messages name its program by its path in this folder
            $text = addr(depth(($raw->($name) // '') =~ s/\Q$prefix\E//gr));
        } elsif ($AS_IS{$name}) {
            $text = $raw->($name);
        } elsif ($ADDRESSED{$name}) {
            $text = addr($raw->($name) // '');
        } elsif ($name eq 'afpd-V.txt') {
            $text = join("\n", grep { index($_, '/') < 0 } lines($raw->($name)));
        }
        $write->($name, $text) if defined $text;
    }
    $write->('README.txt', sprintf($README, POSIX::strftime('%Y-%m-%dT%H:%MZ', gmtime)));

    my @hits = leak_scan($share, $sensitive);
    print STDERR enc(sprintf("netatalk-diag: %s line %d holds '%s'\n", @{$_})) for @hits;
    return @hits && !$force ? 3 : 0;
}

# ---- the Linux sampler ----

# the procfs the sampler reads; the tests give it a tree of their own
our $PROC = '/proc';

# count each thread's state and wait channel at hz until SIGTERM, then write
# 'SAMPLE state wchan count' lines; the file holds its "hz" line once the
# handlers are in place
sub sample {
    my ($pid, $path, $hz) = @_;
    $hz ||= 100;
    my $stop = 0;
    local $SIG{TERM} = sub { $stop = 1 };
    local $SIG{INT}  = 'IGNORE';
    write_text($path, "hz $hz\n");
    my $counts = counter();
    my $base   = "$PROC/$pid/task";

    while (!$stop) {
        opendir(my $dh, $base) or last;
        my @tids = grep {/\A\d+\z/} readdir $dh;
        closedir $dh;
        for my $tid (@tids) {
            my $stat = read_text("$base/$tid/stat");
            next unless defined $stat && $stat =~ /\)\s+(\S+)[^)]*\z/;
            my $state = $1;

            # a procfs without wchan, as NetBSD's is, still gives the state
            my $wchan = trim(read_text("$base/$tid/wchan") // '');
            bump($counts, "$state " . ($state eq 'R' || $wchan eq '0' || $wchan eq '' ? '-' : $wchan));
        }
        Time::HiRes::sleep(1 / $hz);
    }
    write_text($path, join('', "hz $hz\n", map {"SAMPLE $_ $counts->{n}{$_}\n"} most_common($counts)));
    return;
}

# ---- the files of -m ----

# the folder afpd writes the files of -m in; the tests give it one of their own
our $TMP = '/tmp';

# the session's maxdebug logs or dircache dumps: afpd writes them below its
# own /tmp/netatalk-PID, or in /tmp where that folder is not its own. A file
# counts only as a regular file that uid owns and no other name links to,
# opened without following a link, so a file or link another user placed in
# /tmp is never read. --since keeps the files changed at or after FILE's
# change, as file times step in clock ticks; --copy copies each, from the
# file opened, to FOLDER with SUFFIX appended.
sub session_files {
    my ($pid, $uid, $kind, $marker, $copy, $suffix) = @_;
    my $want = $kind eq 'logs' ? qr/\Aafpd\.\Q$pid\E\.[^\/]+\z/ : qr/\Adircache\.\Q$pid\E\z/;
    my @dirs = ($TMP);
    my @st   = lstat "$TMP/netatalk-$pid";
    unshift @dirs, "$TMP/netatalk-$pid" if @st && S_ISDIR($st[2]) && ($st[4] == $uid || $st[4] == 0) && !($st[2] & 022);
    my $since;
    if (defined $marker) {
        $since = (Time::HiRes::stat($marker))[9];
        return () unless defined $since;
    }
    my @found;
    for my $dir (@dirs) {
        for my $name (grep { $_ =~ $want } list_dir($dir)) {
            sysopen(my $fh, "$dir/$name", O_RDONLY | O_NOFOLLOW | O_NONBLOCK) or next;
            my @f = Time::HiRes::stat($fh);
            if (@f && S_ISREG($f[2]) && $f[4] == $uid && $f[3] == 1 && (!defined $since || $f[9] >= $since)) {
                push @found, "$dir/$name";
                copy_from($fh, "$copy/$name" . (defined $suffix ? ".$suffix" : '')) if defined $copy;
            }
            close $fh;
        }
    }
    return @found;
}

sub copy_from {
    my ($fh, $path) = @_;
    binmode $fh;
    my $bytes = do { local $/; <$fh> };
    open(my $out, '>:raw', $path) or die "netatalk-diag: $path: $!\n";
    print {$out} $bytes // '';
    close $out or die "netatalk-diag: $path: $!\n";
    return;
}

# ---- the storage baseline ----

# llistxattr(2), which Perl lacks, by its number on 64-bit x86 and Arm Linux
my %LLISTXATTR = (x86_64 => 195, aarch64 => 12);

sub llistxattr_nr {
    return undef unless $^O eq 'linux' && ($Config{ptrsize} // 0) == 8;
    my ($arch) = ($Config{archname} // '') =~ /\A(\w+)-linux/;
    return defined $arch ? $LLISTXATTR{$arch} : undef;
}

sub now {
    return Time::HiRes::time();
}

# for each volume, time a read-only walk of at most entries entries: listing
# each folder, then each entry's lstat and, where the platform has it, its
# extended attribute names
sub baseline {
    my ($conf, $entries) = @_;
    my $xattr_nr = llistxattr_nr();
    my @out;
    for my $vp (volume_paths(parse_conf(read_text($conf)))) {
        my ($n, $path) = @{$vp};
        $path = enc($path);
        if (!-d $path) {
            push @out, "volume $n: not a folder here";
            next;
        }
        my (%times, @ops);
        my $took = sub {
            my ($op, $t0) = @_;
            push @ops,           $op unless $times{$op};
            push @{$times{$op}}, now() - $t0;
        };
        my @queue = ($path);
        my ($seen, $errors, $t_all) = (0, 0, now());
        while (@queue && $seen < $entries) {
            my $folder = shift @queue;
            my $t0     = now();
            my $dh;
            if (!opendir($dh, $folder)) {
                $errors++;
                next;
            }
            my @items = grep { $_ ne '.' && $_ ne '..' } readdir $dh;
            closedir $dh;
            $took->('list a folder', $t0);
            for my $item (@items) {
                last if $seen >= $entries;
                $seen++;
                my $p = "$folder/$item";
                $t0 = now();
                if (!lstat $p) {
                    $errors++;
                    next;
                }
                $took->('lstat', $t0);
                my $is_dir = -d _;
                if (defined $xattr_nr) {
                    my $buf = "\0" x 65536;
                    $t0 = now();
                    if (syscall($xattr_nr, $p, $buf, length $buf) >= 0) {
                        $took->('list extended attributes', $t0);
                    } else {
                        $errors++;
                    }
                }
                push @queue, $p if $is_dir;
            }
        }
        push @out, sprintf('volume %d: %d entries in %.3f s, %d errors', $n, $seen, now() - $t_all, $errors);
        for my $op (@ops) {
            my $ts = $times{$op};
            push @out,
              sprintf(
                      '   %-26s n %6d, median %7.0f us, p90 %7.0f us, max %8.0f us',
                      $op, scalar @{$ts}, pct($ts, 50) * 1e6, pct($ts, 90) * 1e6, max(@{$ts}) * 1e6
              );
        }
    }
    return @out;
}

# ---- the trace programs ----

my $D_REQUESTS = <<'EOF';
/*
 * Per-request view of one afpd session process, through netatalk's own
 * DTrace provider (built when meson's with-dtrace option is on, the default),
 * on FreeBSD.
 *
 * usage: dtrace -q -s afpd_requests.d -p <afpd session pid> -o requests.txt
 *        stop with ^C or SIGTERM; the summary prints on exit
 *
 * Lines "REQ <end, since start> <command> <in afpd> <on CPU> <asleep>
 * <runnable>", times in usec, give the timeline; a request of 50 ms or more
 * adds a "SLOW" line with its longest sleep. The summary gives, per AFP
 * command, its count and times, the CNID calls it made, why afpd left the
 * CPU (the kernel's wait message or the lock it blocked on), the stacks of
 * sleeps of 1 ms or more, and where it ran, from samples at 997 Hz.
 *
 * A probe on every system call would add its own cost to each request in
 * step with the calls it makes, so the system calls come from
 * afpd_syscalls.d, in a run of their own (netatalk-diag -s). The scheduler
 * probes act only when afpd sleeps and wakes.
 */

#pragma D option quiet
#pragma D option dynvarsize=32m
#pragma D option bufsize=16m
#pragma D option aggsize=16m
#pragma D option stackframes=32

inline int SLOW_NS = 50000000;
inline int STACK_NS = 1000000;

BEGIN
{
	t_begin = timestamp;
}

afp$target:::afpfunc-start
{
	self->name = copyinstr(arg1);
	self->t0 = timestamp;
	self->v0 = vtimestamp;
	self->slept = 0;
	self->waited = 0;
	self->sleeps = 0;
	self->longest = 0;
	self->longwhy = "-";
	self->longcnid = "-";
}

afp$target:::cnid-start
/self->t0/
{
	self->cnid = copyinstr(arg0);
	self->c0 = timestamp;
}

afp$target:::cnid-done
/self->c0/
{
	@cnid[self->name, self->cnid] = count();
	@cnidns[self->name, self->cnid] = sum(timestamp - self->c0);
	self->c0 = 0;
}

/*
 * A thread leaves the CPU with a wait message when it sleeps, with a lock
 * name when it blocks on a kernel lock, and with neither when it is
 * preempted and stays runnable.
 */
sched:::off-cpu
/pid == $target && self->t0/
{
	self->off = timestamp;
	self->why = curthread->td_wmesg != NULL ?
	    stringof(curthread->td_wmesg) :
	    curthread->td_lockname != NULL ?
	    stringof(curthread->td_lockname) :
	    (curthread->td_inhibitors & 0x1) != 0 ? "suspended" : "runnable";
}

/* a woken thread joins a run queue; a preempted one rejoins it unreported */
sched:::enqueue
/args[1]->p_pid == $target/
{
	woken[(int)args[0]->td_tid] = timestamp;
}

sched:::on-cpu
/pid == $target && self->off/
{
	this->away = timestamp - self->off;
	this->waited = self->why == "runnable" ? this->away :
	    woken[(int)tid] > self->off ? timestamp - woken[(int)tid] : 0;
	this->slept = this->away - this->waited;
	this->cnid = self->c0 ? self->cnid : "-";
	self->slept += this->slept;
	self->waited += this->waited;
	self->sleeps += this->slept > 0;
	@offn[self->name, this->cnid, self->why] = count();
	@offs[self->name, this->cnid, self->why] = sum(this->slept);
	@offw[self->name, this->cnid, self->why] = sum(this->waited);
}

sched:::on-cpu
/pid == $target && self->off && this->slept > self->longest/
{
	self->longest = this->slept;
	self->longwhy = self->why;
	self->longcnid = this->cnid;
}

sched:::on-cpu
/pid == $target && self->off && this->slept >= STACK_NS/
{
	@sleepstk[self->name, self->why, stack(), ustack()] = sum(this->slept);
}

sched:::on-cpu
/pid == $target && self->off/
{
	woken[(int)tid] = 0;
	self->off = 0;
}

profile-997
/pid == $target && self->t0 && arg1/
{
	@ucpu[self->name, ufunc(arg1)] = count();
}

profile-997
/pid == $target && self->t0 && arg0/
{
	@kcpu[self->name, stack(10)] = count();
}

afp$target:::afpfunc-done
/self->t0/
{
	this->wall = timestamp - self->t0;
	this->run = vtimestamp - self->v0;
	printf("REQ %d %s %d %d %d %d\n", (timestamp - t_begin) / 1000,
	    self->name, this->wall / 1000, this->run / 1000,
	    self->slept / 1000, self->waited / 1000);
	@n[self->name] = count();
	@ns[self->name] = sum(this->wall);
	@oncpu[self->name] = sum(this->run);
	@asleep[self->name] = sum(self->slept);
	@runnable[self->name] = sum(self->waited);
	@lat[self->name] = quantize(this->wall / 1000);
}

afp$target:::afpfunc-done
/self->t0 && this->wall >= SLOW_NS/
{
	printf("SLOW %d %s %d us: %d on CPU, %d asleep in %d sleeps, ",
	    (timestamp - t_begin) / 1000, self->name, this->wall / 1000,
	    this->run / 1000, self->slept / 1000, self->sleeps);
	printf("%d runnable; longest sleep %d us on %s, CNID call %s\n",
	    self->waited / 1000, self->longest / 1000, self->longwhy,
	    self->longcnid);
}

afp$target:::afpfunc-done
/self->t0/
{
	self->t0 = 0;
}

END
{
	printf("\n== AFP commands: count, total ns\n");
	printa("CMD %-24s %@10d %@16d\n", @n, @ns);
	printf("\n== their time in afpd: on CPU, asleep, runnable, ns\n");
	printa("TIME %-24s %@16d %@16d %@16d\n", @oncpu, @asleep, @runnable);
	printf("\n== CNID calls per AFP command: count, total ns\n");
	printa("CNID %-24s %-24s %@10d %@16d\n", @cnid, @cnidns);
	printf("\n== off CPU per AFP command, CNID call and reason: ");
	printf("count, ns asleep, ns runnable\n");
	printa("OFF %-24s %-16s %-24s %@10d %@16d %@16d\n", @offn, @offs,
	    @offw);
	printf("\n== sleeps of 1 ms or more per AFP command and reason, ");
	printf("by kernel and user stack: ns\n");
	trunc(@sleepstk, 12);
	printa(@sleepstk);
	printf("\n== on CPU in requests, samples at 997 Hz: user functions\n");
	trunc(@ucpu, 20);
	printa(@ucpu);
	printf("\n== on CPU in requests, samples at 997 Hz: kernel stacks\n");
	trunc(@kcpu, 12);
	printa(@kcpu);
	printf("\n== AFP command latency in afpd, usec\n");
	printa(@lat);
}
EOF

my $D_SYSCALLS = <<'EOF';
/*
 * The system calls of one afpd session process on FreeBSD: in a run of
 * their own, as these probes add to each request's time, or alone for an
 * afpd built without netatalk's DTrace provider.
 *
 * usage: dtrace -q -s afpd_syscalls.d -p <afpd session pid> -o syscalls.txt
 *        stop with ^C or SIGTERM; the summary prints on exit
 *
 * Per system call: count, total time, and a latency histogram for the calls
 * that walk paths (chdir, __getcwd, openat, fstatat and the extended
 * attribute and ACL calls).
 */

#pragma D option quiet
#pragma D option dynvarsize=16m
/* a system call this FreeBSD release lacks has no probe */
#pragma D option zdefs

syscall:::entry
/pid == $target/
{
	self->s0 = timestamp;
}

syscall:::return
/pid == $target && self->s0/
{
	@n[probefunc] = count();
	@ns[probefunc] = sum(timestamp - self->s0);
	self->s0 = 0;
}

syscall::chdir:entry, syscall::fchdir:entry, syscall::__getcwd:entry,
syscall::openat:entry, syscall::fstatat:entry,
syscall::extattr_get_file:entry, syscall::extattr_get_link:entry,
syscall::extattr_get_fd:entry, syscall::extattr_list_file:entry,
syscall::extattr_list_link:entry, syscall::extattr_list_fd:entry,
syscall::__acl_get_file:entry, syscall::__acl_get_link:entry,
syscall::__acl_get_fd:entry
/pid == $target/
{
	self->w0 = timestamp;
}

syscall::chdir:return, syscall::fchdir:return, syscall::__getcwd:return,
syscall::openat:return, syscall::fstatat:return,
syscall::extattr_get_file:return, syscall::extattr_get_link:return,
syscall::extattr_get_fd:return, syscall::extattr_list_file:return,
syscall::extattr_list_link:return, syscall::extattr_list_fd:return,
syscall::__acl_get_file:return, syscall::__acl_get_link:return,
syscall::__acl_get_fd:return
/self->w0/
{
	@lat[probefunc] = quantize((timestamp - self->w0) / 1000);
	self->w0 = 0;
}

END
{
	printf("== system calls: count, total ns\n");
	printa("SYS %-24s %@10d %@16d\n", @n, @ns);
	printf("\n== path-walking calls, usec\n");
	printa(@lat);
}
EOF

my $D_ZFS = <<'EOF';
/*
 * What one afpd session's path lookups cost ZFS on FreeBSD: name-cache
 * misses, reverse lookups from a vnode to its name, and the directory scans
 * those take.
 *
 * usage: dtrace -q -s zfs_lookups.d -o zfs.txt PID
 *        PID is the session's afpd; stop with ^C or SIGTERM, and the summary
 *        prints on exit
 *
 * zfs_freebsd_cachedlookup  a forward lookup the name cache could not answer
 * zfs_lookup                every forward lookup ZFS serves: those misses,
 *                           and all of them on a case-insensitive or
 *                           normalising dataset, which keeps no name cache
 * zfs_vptocnp               a reverse lookup getcwd() could not take from
 *                           the name cache
 * zap_value_search          the scan of a parent folder's entries that a
 *                           reverse lookup takes, its time the cost
 * vn_fullpath_dir           getcwd()'s locked walk up the name cache, taken
 *                           only when its lockless walk fails
 */

#pragma D option quiet
/* a function this kernel inlines or names otherwise has no probe */
#pragma D option zdefs

/* every function prints, at 0 when nothing called it */
BEGIN
{
	@n["zfs_freebsd_cachedlookup"] = sum(0);
	@n["zfs_lookup"] = sum(0);
	@n["zfs_vptocnp"] = sum(0);
	@n["zap_value_search"] = sum(0);
	@n["vn_fullpath_dir"] = sum(0);
	@ns["zap_value_search"] = sum(0);
	@ns["vn_fullpath_dir"] = sum(0);
}

fbt::zfs_freebsd_cachedlookup:entry
/pid == $1/
{
	@n["zfs_freebsd_cachedlookup"] = sum(1);
}

fbt::zfs_lookup:entry
/pid == $1/
{
	@n["zfs_lookup"] = sum(1);
}

fbt::zfs_vptocnp:entry
/pid == $1/
{
	@n["zfs_vptocnp"] = sum(1);
}

fbt::zap_value_search:entry
/pid == $1/
{
	self->z0 = timestamp;
}

fbt::zap_value_search:return
/self->z0/
{
	@n["zap_value_search"] = sum(1);
	@ns["zap_value_search"] = sum(timestamp - self->z0);
	@scan = quantize((timestamp - self->z0) / 1000);
	self->z0 = 0;
}

fbt::vn_fullpath_dir:entry
/pid == $1/
{
	self->f0 = timestamp;
}

fbt::vn_fullpath_dir:return
/self->f0/
{
	@n["vn_fullpath_dir"] = sum(1);
	@ns["vn_fullpath_dir"] = sum(timestamp - self->f0);
	self->f0 = 0;
}

END
{
	printf("== kernel functions called for afpd: count\n");
	printa("ZFS %-28s %@10d\n", @n);
	printf("\n== their time, ns\n");
	printa("ZFSNS %-28s %@16d\n", @ns);
	printf("\n== zap_value_search scans, usec\n");
	printa(@scan);
}
EOF

# bpftrace has no thread-local variables: maps keyed by thread stand in for
# them, and every request deletes its own entries
my $BT_REQUESTS = <<'EOF';
/*
 * Per-request view of one afpd session process on Linux, through
 * netatalk's USDT probes: the "REQ" and "SLOW" lines of afpd_requests.d.
 * A switch out with state 0 or a preemption is runnable time, any other
 * state sleep; sleeps of 1 ms or more keep their kernel stack.
 *
 * The maps are keyed by the kernel's thread ID, curtask->pid, which the
 * scheduler's tracepoints carry too: bpftrace's pid and tid are the IDs in
 * its own pid namespace, and in a container they differ, and fail for
 * every task outside it.
 *
 * usage: bpftrace -q -p <pid> afpd_requests.bt > requests.txt
 *        netatalk_diag.pl program writes the session's pid and the afpd
 *        binary into the probes
 */

BEGIN
{
	@begin = nsecs;
	printf("STARTED\n");
}

usdt:AFPD:afp:afpfunc__start
/pid == PID/
{
	$k = curtask->pid;
	@name[$k] = str(arg1);
	@t0[$k] = nsecs;
	@slept[$k] = (uint64)0;
	@waited[$k] = (uint64)0;
	@sleeps[$k] = (uint64)0;
	@longest[$k] = (uint64)0;
}

usdt:AFPD:afp:cnid__start
/pid == PID && @t0[curtask->pid]/
{
	$k = curtask->pid;
	@cname[$k] = str(arg0);
	@c0[$k] = nsecs;
}

usdt:AFPD:afp:cnid__done
/pid == PID && @c0[curtask->pid]/
{
	$k = curtask->pid;
	@cnid[@name[$k], @cname[$k]] = count();
	@cnidns[@name[$k], @cname[$k]] = sum(nsecs - @c0[$k]);
	delete(@c0[$k]);
	delete(@cname[$k]);
}

tracepoint:sched:sched_switch
/@t0[curtask->pid]/
{
	$k = curtask->pid;
	@off[$k] = nsecs;
	@state[$k] = (args.prev_state & 0xff) == 0 ? 0 :
	    ((args.prev_state & 2) ? 2 : 1);
	@stk[$k] = kstack(10);
}

tracepoint:sched:sched_wakeup
/@off[args.pid]/
{
	@woken[args.pid] = nsecs;
}

tracepoint:sched:sched_switch
/@off[args.next_pid]/
{
	$t = args.next_pid;
	$away = nsecs - @off[$t];
	/* bpftrace 0.23 crashes on a nested conditional expression here */
	$waited = (uint64)0;
	if (@state[$t] == 0) {
		$waited = $away;
	} else {
		if (@woken[$t] > @off[$t]) {
			$waited = nsecs - @woken[$t];
		}
	}
	$slept = $away - $waited;
	@slept[$t] += $slept;
	@waited[$t] += $waited;
	if ($slept > (uint64)0) {
		@sleeps[$t] += (uint64)1;
	}
	if ($slept > @longest[$t]) {
		@longest[$t] = $slept;
	}
	if (@state[$t] == 2) {
		@offs[@name[$t], "D"] = sum($slept);
	}
	if (@state[$t] == 1) {
		@offs[@name[$t], "S"] = sum($slept);
	}
	if ($slept >= (uint64)1000000) {
		@sleepstk[@name[$t], @stk[$t]] = sum($slept);
	}
	delete(@off[$t]);
	delete(@woken[$t]);
	delete(@state[$t]);
	delete(@stk[$t]);
}

usdt:AFPD:afp:afpfunc__done
/pid == PID && @t0[curtask->pid]/
{
	$k = curtask->pid;
	$us = (uint64)1000;
	$wall = nsecs - @t0[$k];
	$cpu = $wall - @slept[$k] - @waited[$k];
	printf("REQ %llu %s %llu %llu %llu %llu\n", (nsecs - @begin) / $us,
	    @name[$k], $wall / $us, $cpu / $us, @slept[$k] / $us,
	    @waited[$k] / $us);
	if ($wall >= (uint64)50000000) {
		printf("SLOW %llu %s %llu us: %llu on CPU, ",
		    (nsecs - @begin) / $us, @name[$k], $wall / $us, $cpu / $us);
		printf("%llu asleep in %llu sleeps, %llu runnable; ",
		    @slept[$k] / $us, @sleeps[$k], @waited[$k] / $us);
		printf("longest sleep %llu us\n", @longest[$k] / $us);
	}
	@n[@name[$k]] = count();
	@ns[@name[$k]] = sum($wall);
	@oncpu[@name[$k]] = sum($cpu);
	@asleep[@name[$k]] = sum(@slept[$k]);
	@runnable[@name[$k]] = sum(@waited[$k]);
	delete(@name[$k]);
	delete(@t0[$k]);
	delete(@slept[$k]);
	delete(@waited[$k]);
	delete(@sleeps[$k]);
	delete(@longest[$k]);
}

END
{
	clear(@begin);
	clear(@name);
	clear(@t0);
	clear(@slept);
	clear(@waited);
	clear(@sleeps);
	clear(@longest);
	clear(@off);
	clear(@state);
	clear(@stk);
	clear(@woken);
	clear(@cname);
	clear(@c0);
}
EOF

my $BT_SYSCALLS = <<'EOF';
/*
 * The system calls of one afpd session process on Linux: count and total
 * time per call, in a run of their own, as these probes add to each
 * request's time.
 *
 * The process name comes first: bpftrace's pid is the ID in its own pid
 * namespace, which fails for every task outside it.
 *
 * usage: bpftrace -q -p <pid> afpd_syscalls.bt > syscalls.txt
 */

BEGIN
{
	printf("STARTED\n");
}

tracepoint:syscalls:sys_enter_*
/comm == "afpd" && pid == PID/
{
	@s0[curtask->pid] = nsecs;
}

tracepoint:syscalls:sys_exit_*
/@s0[curtask->pid]/
{
	@n[probe] = count();
	@ns[probe] = sum(nsecs - @s0[curtask->pid]);
	delete(@s0[curtask->pid]);
}

END
{
	clear(@s0);
}
EOF

our %PROGRAMS = (
                 'afpd_requests.d'  => $D_REQUESTS,
                 'afpd_syscalls.d'  => $D_SYSCALLS,
                 'zfs_lookups.d'    => $D_ZFS,
                 'afpd_requests.bt' => $BT_REQUESTS,
                 'afpd_syscalls.bt' => $BT_SYSCALLS,
);

# a program's text; a bpftrace program gets the session's pid and the afpd
# binary, which bpftrace cannot take on its command line
sub program {
    my ($name, $pid, $afpd) = @_;
    my $text = $PROGRAMS{$name};
    if ($name =~ /\.bt\z/) {
        $text =~ s/PID/$pid/g;
        $text =~ s/AFPD/$afpd/g;
    }
    return $text;
}

my $USAGE = <<'EOF';
usage: netatalk_diag.pl report OUTDIR FOLDER [--force]
       netatalk_diag.pl pcap CAPTURE [--csv FILE] [--keep-names] [--port N]
       netatalk_diag.pl volumes AFPCONF MOUNTS
       netatalk_diag.pl sample PID FILE
       netatalk_diag.pl baseline AFPCONF ENTRIES
       netatalk_diag.pl session-files PID UID logs|dumps [--since FILE]
                        [--copy FOLDER [--suffix SUFFIX]]
       netatalk_diag.pl program NAME [--pid PID] [--afpd PATH]
EOF

sub pcap_command {
    my ($capture, %o)    = @_;
    my ($conns,   $rows) = analyse_capture($capture, $o{port}, new_names($o{'keep-names'}));
    write_csv($rows, $o{csv}) if defined $o{csv};
    my @out;
    report_requests($rows, \@out);
    verdicts($rows, \@out);
    tcp_health($conns, \@out);
    transfers($rows, \@out);
    logins($conns, $rows, \@out);
    print join("\n", @out), "\n";

    for my $conn (@{$conns}) {
        for my $side (grep { $conn->{$_}{gaps} } qw(c2s s2c)) {
            print STDERR "warning: $conn->{label} $side: lost bytes, frames after the loss are skipped; capture with "
              . "a larger tcpdump -B\n";
        }
    }
    return 0;
}

sub main {
    my ($command, @args) = @_;
    my %o = (port => 548, pid => 0, afpd => '/usr/sbin/afpd');
    Getopt::Long::GetOptionsFromArray(
                                      \@args, \%o,
                                      qw(force csv=s keep-names port=i since=s copy=s suffix=s pid=i afpd=s)
    ) or return usage();
    binmode STDOUT, ':encoding(UTF-8)';
    $command //= '';
    my $n = @args;
    return report($args[0], $args[1], $o{force}) if $command eq 'report' && $n == 2;
    return pcap_command($args[0], %o)            if $command eq 'pcap'   && $n == 1;

    if ($command eq 'volumes' && $n == 2) {
        my $mounts = read_mounts(read_text($args[1]));
        for my $vp (volume_paths(parse_conf(read_text($args[0])))) {
            my $m = mount_of($vp->[1], $mounts) or next;
            print "$vp->[0]\t$m->[1]\t$m->[2]\n";
        }
        return 0;
    }
    if ($command eq 'sample' && $n == 2 && is_count($args[0])) {
        sample(@args);
        return 0;
    }
    if ($command eq 'baseline' && $n == 2 && is_count($args[1])) {
        print map {"$_\n"} baseline(@args);
        return 0;
    }
    if (    $command eq 'session-files'
         && $n == 3
         && is_count($args[0])
         && is_count($args[1])
         && $args[2] =~ /\A(logs|dumps)\z/) {
        print map {"$_\n"} session_files(@args, @o{qw(since copy suffix)});
        return 0;
    }
    if ($command eq 'program' && $n == 1 && exists $PROGRAMS{$args[0]}) {
        print program($args[0], $o{pid}, $o{afpd});
        return 0;
    }
    return usage();
}

sub usage {
    print STDERR $USAGE;
    return 2;
}

package main;

exit NetatalkDiag::main(@ARGV) unless caller;

1;
