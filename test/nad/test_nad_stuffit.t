#!/usr/bin/perl

# StuffIt CLI round trips for nad. Meson registers this only with StuffIt enabled.
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
# TODO: Re-enable after issue #3340, item 4 is fixed; the fixtures use mkdir.
# https://github.com/Netatalk/netatalk/issues/3340
plan skip_all => 'AppleDouble v2 mkdir crash (issue #3340, item 4)'
    if $fixture->uses_adouble_v2;
my $data = pack('C*', map { ($_ * 23 + 5) & 0xff } 0 .. 9000);
my $resource = pack('C*', map { ($_ * 11 + 17) & 0xff } 0 .. 4000);
my $source = $fixture->seed_forked_file('payload', $data, $resource,
    'TEXT', 'NADT');

sub check_extracted_resource {
    my ($dir, $label) = @_;
    my $file = "$dir/payload";
    is($fixture->file_contents($file), $data, "$label restores the data fork");

    my $listed = $fixture->succeeds_at("$label reads Finder metadata",
        $dir, 'ls', '-l', $file);
    like($listed->{out}, qr/\bTEXT NADT\b/,
        "$label restores Finder type and creator");

    $fixture->succeeds_at("$label exports restored resource fork", $dir,
        'bin', '--filename', 'verify', $file);
    my $binary = $fixture->file_contents("$dir/verify.bin");
    ok(defined $binary && length($binary) >= 128,
        "$label produces a MacBinary verification file");
    return unless defined $binary && length($binary) >= 128;

    is(unpack('N', substr($binary, 87, 4)), length($resource),
        "$label restores resource fork length");
    my $offset = 128 + int((length($data) + 127) / 128) * 128;
    is(substr($binary, $offset, length($resource)), $resource,
        "$label restores resource fork bytes");
}

note('default StuffIt method');
$fixture->succeeds_at('sit creates a single-file archive',
    $volume, 'sit', $source);
my $single_archive = "$volume/payload.sit";
ok(-s $single_archive, 'default archive is nonempty');

my $single_extract = "$volume/single_extract";
$fixture->succeeds('mkdir creates extraction directory',
    'mkdir', $single_extract);
$fixture->succeeds_at('unsit extracts a single file',
    $single_extract, 'unsit', $single_archive);
check_extracted_resource($single_extract, 'default method');

my $overwrite = $fixture->run_at($single_extract, 'unsit', $single_archive);
isnt($overwrite->{status}, 0, 'unsit refuses to overwrite an existing file');
is($fixture->file_contents("$single_extract/payload"), $data,
    'refused overwrite leaves the original data intact');

note('multiple inputs and nested directories');
my $tree = "$volume/tree";
$fixture->succeeds('mkdir creates a source directory', 'mkdir', $tree);
my $child = $fixture->seed_forked_file('child', "nested data\n", '',
    'TEXT', 'NADT');
$fixture->succeeds('mv places a file in the source directory',
    'mv', $child, "$tree/child");

my $no_output = $fixture->run_at($volume, 'sit', $source, $tree);
isnt($no_output->{status}, 0, 'multiple sit inputs require -o');

my $bundle = "$volume/bundle.sit";
$fixture->succeeds_at('sit --method 0 archives a file and directory',
    $volume, 'sit', '--method', '0', '-o', $bundle, $source, $tree);
ok(-s $bundle, 'multi-input archive is nonempty');

my $bundle_extract = "$volume/bundle_extract";
$fixture->succeeds('mkdir creates second extraction directory',
    'mkdir', $bundle_extract);
$fixture->succeeds_at('unsit extracts multiple entries',
    $bundle_extract, 'unsit', $bundle);
check_extracted_resource($bundle_extract, 'uncompressed method');
is($fixture->file_contents("$bundle_extract/tree/child"), "nested data\n",
    'nested file contents survive the archive');

note('invalid input');
my $invalid = $fixture->work . '/invalid.sit';
$fixture->write_file($invalid, "not a StuffIt archive\n");
isnt($fixture->run_at($volume, 'unsit', $invalid)->{status}, 0,
    'unsit rejects invalid archive data');
isnt($fixture->run_at($volume, 'sit', '--method', '999', $source)->{status}, 0,
    'sit rejects an out-of-range method');

done_testing;
