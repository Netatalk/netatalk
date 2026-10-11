#!/usr/bin/perl

# MacBinary and BinHex CLI round trips for nad.
# Copyright (C) 2026 Daniel Markstedt <daniel@mindani.net>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.

use strict;
use warnings;

use FindBin;
use lib $FindBin::Bin;
use NadTest;
use Test::More;

my $fixture = NadTest->new(shift @ARGV);
my $volume = $fixture->volume;
my $data = pack('C*', map { ($_ * 37 + 11) & 0xff } 0 .. 9000);
my $resource = pack('C*', map { ($_ * 19 + 3) & 0xff } 0 .. 5000);
my $source = $fixture->seed_forked_file('forked', $data, $resource,
    'TEXT', 'NADT');

sub check_macbinary {
    my ($path, $name, $label) = @_;
    my $bytes = $fixture->file_contents($path);
    ok(defined $bytes && length($bytes) >= 128, "$label has a MacBinary header");
    return unless defined $bytes && length($bytes) >= 128;

    my $name_length = unpack('C', substr($bytes, 1, 1));
    is(substr($bytes, 2, $name_length), $name, "$label stores its filename");
    is(substr($bytes, 65, 8), 'TEXTNADT', "$label stores Finder type and creator");
    is(unpack('N', substr($bytes, 83, 4)), length($data),
        "$label stores the data fork length");
    is(unpack('N', substr($bytes, 87, 4)), length($resource),
        "$label stores the resource fork length");
    is(substr($bytes, 128, length($data)), $data,
        "$label preserves the data fork bytes");
    my $resource_offset = 128 + int((length($data) + 127) / 128) * 128;
    is(substr($bytes, $resource_offset, length($resource)), $resource,
        "$label preserves the resource fork bytes");
}

note('MacBinary');
is($fixture->file_contents($source), $data, 'seed has the expected data fork');
my $bin_source_header = $fixture->succeeds_at('bin --header reads source metadata',
    $volume, 'bin', '--header', $source);
like($bin_source_header->{out}, qr/creator:\s+'NADT'/,
    'source header has the expected creator');
like($bin_source_header->{out}, qr/fork length\[1\]:\s+5001\b/,
    'source header has the expected resource fork length');

$fixture->succeeds_at('bin encodes the file', $volume, 'bin', $source);
my $bin = "$volume/forked.bin";
check_macbinary($bin, 'forked', 'MacBinary output');
my $bin_header = $fixture->succeeds_at('unbin --header reads the archive',
    $volume, 'unbin', '--header', $bin);
like($bin_header->{out}, qr/name:\s+forked\b/,
    'unbin reads the encoded filename');

$fixture->succeeds_at('rm removes the source before extraction',
    $volume, 'rm', $source);
$fixture->succeeds_at('unbin restores the file', $volume, 'unbin', $bin);
is($fixture->file_contents($source), $data, 'unbin restores the data fork');
$fixture->succeeds_at('bin re-encodes the imported file', $volume,
    'bin', '--filename', 'after_bin', $source);
check_macbinary("$volume/after_bin.bin", 'after_bin', 'MacBinary round trip');

note('BinHex');
$fixture->succeeds_at('hex encodes the file', $volume, 'hex', $source);
my $hex = "$volume/forked.hqx";
my $hex_bytes = $fixture->file_contents($hex);
like($hex_bytes, qr/^\(This file must be converted with BinHex 4\.0\)/,
    'BinHex output has the format preamble');
my $hex_header = $fixture->succeeds_at('unhex --header reads the archive',
    $volume, 'unhex', '--header', $hex);
like($hex_header->{out}, qr/fork length\[1\]:\s+5001\b/,
    'BinHex archive contains the resource fork');

$fixture->succeeds_at('rm removes the source before BinHex extraction',
    $volume, 'rm', $source);
$fixture->succeeds_at('unhex restores the file', $volume, 'unhex', $hex);
is($fixture->file_contents($source), $data, 'unhex restores the data fork');
$fixture->succeeds_at('bin exports the BinHex-imported file', $volume,
    'bin', '--filename', 'after_hex', $source);
check_macbinary("$volume/after_hex.bin", 'after_hex', 'BinHex round trip');

note('invalid input');
my $invalid = $fixture->work . '/invalid.dat';
$fixture->write_file($invalid, "not an archive\n");
isnt($fixture->run_at($volume, 'unbin', $invalid)->{status}, 0,
    'unbin rejects an invalid archive');
isnt($fixture->run_at($volume, 'unhex', $invalid)->{status}, 0,
    'unhex rejects an invalid archive');

done_testing;
