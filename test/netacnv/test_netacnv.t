#!/usr/bin/perl

# Copyright (c) 2026 Daniel Markstedt <daniel@mindani.net>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.

use strict;
use warnings;
use IO::Select;
use IPC::Open3;
use Symbol qw(gensym);
use Test::More;

my $exe = shift @ARGV or BAIL_OUT('netacnv executable required');

# Drain both pipes concurrently, including verbose hex output, without a shell
# or assumptions about the terminal's encoding.
sub run {
    my (@args) = @_;
    my ($in, $out);
    my $err = gensym;
    my $pid = open3($in, $out, $err, $exe, @args);
    close $in;
    binmode $out;
    binmode $err;
    my $stdout = '';
    my $stderr = '';
    my $out_fd = fileno $out;
    my $select = IO::Select->new($out, $err);

    while ($select->count) {
        for my $fh ($select->can_read) {
            my $count = sysread($fh, my $bytes, 4096);
            defined $count or BAIL_OUT("cannot read child output: $!");

            if ($count == 0) {
                $select->remove($fh);
                close $fh;
            } elsif (fileno($fh) == $out_fd) {
                $stdout .= $bytes;
            } else {
                $stderr .= $bytes;
            }
        }
    }

    waitpid($pid, 0) == $pid or BAIL_OUT("cannot wait for child: $!");
    return ($?, $stdout, $stderr);
}

sub check {
    my ($name, $args, $exit, $expected_stdout, $expected_stderr) = @_;
    my ($status, $stdout, $stderr) = run(@$args);
    is($status, $exit << 8,       "$name: exit status");
    is($stdout, $expected_stdout, "$name: stdout");

    if (ref $expected_stderr eq 'Regexp') {
        like($stderr, $expected_stderr, "$name: stderr diagnostic");
    } else {
        is($stderr, $expected_stderr, "$name: stderr");
    }
}

# Probe the selected implementation through netacnv. By default, require both
# roles; callers can request only the role their checks exercise. Only a charset
# loading diagnostic means unavailable, so other failures still reach the tests.
sub charset_available {
    my ($charset, @roles) = @_;
    @roles = qw(source target) unless @roles;

    for my $role (@roles) {
        my $option = $role eq 'source' ? '-f' : '-t';
        my ($status, undef, $stderr) = run('-x', $option, $charset, '');
        return 0 if $status && $stderr =~ /cannot load \Q$role\E charset/;
    }

    return 1;
}

my $utf16le_source_available = charset_available('UTF-16LE', 'source');
my $utf16le_target_available = charset_available('UTF-16LE', 'target');

my ($help_status, $help, $help_err) = run('-h');
is($help_status, 0, 'help succeeds without an input string');
like($help, qr/^Usage: netacnv/m, 'help goes to stdout');
is($help_err, '', 'help has no error output');
my ($maximum) = $help =~ /limited to (\d+) bytes/;
defined $maximum or BAIL_OUT('help must state the conversion size limit');

check('default conversion', ['hello'], 0, "from: hello\nto: hello\n", '');
check('empty input',        [''],      0, "from: \nto: \n",           '');
check('option-like string after --', ['--', '-hello'], 0,
      "from: -hello\nto: -hello\n",  '');
check('no input',                [],                 1, '', qr/exactly one input string/);
check('extra input',             ['hello', 'world'], 1, '', qr/exactly one input string/);
check('unknown CLI option',      ['-q', 'hello'],    1, '', qr/Usage:/);
check('missing option argument', ['-f'],             1, '', qr/Usage:/);
check('unknown conversion flag', ['-o', 'CONV_BOGUS', 'hello'], 1, '',
      qr/unknown conversion option 'CONV_BOGUS'/);
check(
      'conflicting case flags',
      ['-o', 'CONV_TOUPPER', '-o', 'CONV_TOLOWER', 'hello'], 1, '',
      qr/conflicting conversion options/
);
check(
      'conflicting composition flags',
      ['-o', 'CONV_PRECOMPOSE', '-o', 'CONV_DECOMPOSE', 'hello'], 1, '',
      qr/conflicting conversion options/
);

for my $option (qw(f t m)) {
    check("invalid -$option charset", ["-$option", 'NO_SUCH_CHARSET', 'hello'],
          1, '', qr/cannot load .* charset 'NO_SUCH_CHARSET'/);
    check("empty -$option charset", ["-$option", '', 'hello'],
          1, '', qr/charset name must not be empty/);
}

check('last source option wins', ['-f', 'NO_SUCH_CHARSET', '-f', 'UTF8', 'hello'],
      0, "from: hello\nto: hello\n", '');
check(
      'repeated flags accumulate',
      ['-f', 'UTF8', '-o', 'CONV_TOUPPER', '-o', 'CONV_ESCAPEDOTS', '.hello'],
      0, "from: .hello\nto: :2EHELLO\n", ''
);
check('native UCS-2 hex input', ['-x', '-f', 'UCS-2', unpack('H*', pack('S', 0x4e2d))],
      0, "from: " . pack('S', 0x4e2d) . "\nto: \xe4\xb8\xad\n", '');
for my $case (
              ['UTF-16LE', '41003a002f002e012f013a01'],
              ['UTF-16BE', '0041003a002f012e012f013a'],
              ['UTF-16',   'feff0041003a002f012e012f013a'],
              ['UTF-32LE', '410000003a0000002f0000002e0100002f0100003a010000'],
              ['UTF-32BE', '000000410000003a0000002f0000012e0000012f0000013a'],
              ['UCS-2LE',  '41003a002f002e012f013a01'],
              ['UCS-2BE',  '0041003a002f012e012f013a'],
              ['utf16le',  '41003a002f002e012f013a01']
) {
    my ($charset, $hex) = @$case;
  SKIP: {
        skip "$charset is unavailable as a source charset on this platform", 6
          unless charset_available($charset, 'source');
        check("$charset decodes before punctuation scanning", ['-x', '-f', $charset, $hex],
              0, "from: " . pack('H*', $hex) . "\nto: A:/\xc4\xae\xc4\xaf\xc4\xba\n", '');
        check("$charset accepts empty input", ['-x', '-f', $charset, ''],
              0, "from: \nto: \n", '');
    }
}
SKIP: {
    skip 'UTF-16LE is unavailable as a source charset on this platform', 3
      unless $utf16le_source_available;
    check('raw wide input remains supported', ['-f', 'UTF-16LE', "\x2f\x01"],
          0, "from: \x2f\x01\nto: \xc4\xaf\n", '');
}
check('hex accepts whitespace between mixed-case byte pairs', ['-x', " c3\tA9 \n"],
      0, "from: \xc3\xa9\nto: \xc3\xa9\n", '');
check('hex preserves embedded NULs', ['-x', '-f', 'UTF8', '610062'],
      0, "from: a\0b\nto: a\0b\n", '');
check('hex empty input',                      ['-x', ''],        0, "from: \nto: \n", '');
check('hex whitespace-only input',            ['-x', " \t\r\n"], 0, "from: \nto: \n", '');
check('hex rejects odd digit count',          ['-x', '410'],     1, '',               qr/complete byte pairs/);
check('hex rejects whitespace inside a byte', ['-x', '4 1'],     1, '',               qr/complete byte pairs/);
check('hex rejects non-hex characters',       ['-x', '41zz'],    1, '', qr/invalid character in hex input/);
check('hex input limit counts decoded bytes', ['-x', '61' x $maximum],
      0, 'from: ' . ('a' x $maximum) . "\nto: " . ('a' x $maximum) . "\n", '');
check('hex rejects oversized decoded input', ['-x', '61' x ($maximum + 1)],
      1, '', qr/hex input exceeds maximum of $maximum bytes/);
SKIP: {
    skip 'UTF-16LE is unavailable as a source charset on this platform', 30
      unless $utf16le_source_available;
    check(
          'wide input handles actual colon and slash escaping',
          ['-x', '-f', 'UTF-16LE', '-o', 'CONV_ESCAPEHEX', '3a002f00'],
          0, "from: :\0/\0\nto: :3a:2f\n", ''
    );
    check(
          'wide input handles leading dots',
          ['-x', '-f', 'UTF-16LE', '-o', 'CONV_ESCAPEDOTS', '2e006100'],
          0, "from: .\0a\0\nto: :2ea\n", ''
    );
    check(
          'wide input handles hex unescaping',
          ['-x', '-f', 'UTF-16LE', '-o', 'CONV_UNESCAPEHEX', '3a0038006500'],
          0, "from: :\x008\0e\0\nto: \xc3\xa9\n", ''
    );
    check(
          'wide input keeps Mac target punctuation mapping',
          ['-x', '-f', 'UTF-16LE', '-t', 'MAC_ROMAN', '61003a006200'],
          0, "from: a\0:\0b\0\nto: a/b\n", ''
    );
    check(
          'wide verbose bytes exclude terminators but include encoded NULs',
          ['-v', '-x', '-f', 'UTF-16LE', '4100'],
          0, "from: A\0\nto: A\n",
          qr/from bytes \(2 bytes\): 41 00\nto bytes \(1 bytes\): 41\nflags: 0x0000/
    );
    check('wide truncated source fails in the library', ['-x', '-f', 'UTF-16LE', '410042'],
          1, '', qr/conversion from 'UTF-16LE' to 'UTF8' failed:/);
    check(
          'wide invalid source respects CONV_IGNORE',
          ['-v', '-x', '-f', 'UTF-16LE', '-o', 'CONV_IGNORE', '410000d84200'],
          0, "from: A\0\0\xd8B\0\nto: A\n", qr/CONV_IGNORE CONV_REQMANGLE/
    );
    check(
          'wide invalid source respects CONV_FORCE',
          ['-x', '-f', 'UTF-16LE', '-o', 'CONV_FORCE', '410000d84200'],
          0, "from: A\0\0\xd8B\0\nto: A\n", ''
    );
    check(
          'wide invalid source replacement keeps code-unit alignment',
          ['-x', '-f', 'UTF-16LE', '-o', 'CONV__EILSEQ', '410000d84200'],
          0, "from: A\0\0\xd8B\0\nto: A_B\n", ''
    );
    check(
          'wide truncated source respects CONV__EILSEQ',
          ['-x', '-f', 'UTF-16LE', '-o', 'CONV__EILSEQ', '410042'],
          0, "from: A\0B\nto: A_\n", ''
    );
}
check(
      'single-byte iconv source preserves non-ASCII characters and punctuation',
      ['-f', 'ISO-8859-1', "\xe9:/"],   0,
      "from: \xe9:/\nto: \xc3\xa9:/\n", ''
);
SKIP: {
    skip 'UTF-16LE is unavailable as a target charset on this platform', 3
      unless $utf16le_target_available;
    check('wide target is printed by byte length', ['-f', 'UTF8', '-t', 'UTF-16LE', 'AB'],
          0, "from: AB\nto: A\0B\0\n", '');
}
check(
      'escape charset does not make ASCII a Mac target',
      ['-f', 'UTF8', '-t', 'ASCII', '-m', 'ASCII', 'a:b'],
      0, "from: a:b\nto: a:b\n", ''
);
check(
      'escape charset does not make ASCII a Mac source',
      ['-f', 'ASCII', '-t', 'UTF8', '-m', 'ASCII', 'a/b'],
      0, "from: a/b\nto: a/b\n", ''
);
check(
      'Mac Roman punctuation mapping survives a different escape charset',
      ['-f', 'MAC_ROMAN', '-t', 'UTF8', '-m', 'ASCII', 'a/b'],
      0, "from: a/b\nto: a:b\n", ''
);
check(
      'verbose normalization shows exact bytes', ['-v', "e\xcc\x81"],
      0,                                         "from: e\xcc\x81\nto: \xc3\xa9\n",
      qr/from bytes \(3 bytes\): 65 cc 81\nto bytes \(2 bytes\): c3 a9\nflags: 0x0000/
);
check(
      'verbose flags expose a partial conversion',
      ['-v', '-f', 'UTF8', '-t', 'ASCII', '-o', 'CONV_IGNORE', "a\xc3\xa9z"],
      0, "from: a\xc3\xa9z\nto: a\n", qr/CONV_IGNORE CONV_REQMANGLE/
);
check(
      'verbose flags expose hex escaping',
      ['-v', '-f', 'UTF8', '-t', 'ASCII', '-o', 'CONV_ESCAPEHEX', "\xc3\xa9"],
      0, "from: \xc3\xa9\nto: :8e\n", qr/CONV_ESCAPEHEX CONV_REQESCAPE/
);
check(
      'hex escaping uses the selected escape charset',
      ['-f', 'UTF8', '-t', 'ASCII', '-m', 'UTF8', '-o', 'CONV_ESCAPEHEX', "\xc3\xa9"],
      0, "from: \xc3\xa9\nto: :c3:a9\n", ''
);
check(
      'hex unescaping uses the selected escape charset',
      ['-f', 'UTF8', '-t', 'UTF8', '-m', 'UTF8', '-o', 'CONV_UNESCAPEHEX', ':c3:a9'],
      0, "from: :c3:a9\nto: \xc3\xa9\n", ''
);
check('invalid UTF8 fails', ['-f', 'UTF8', "\xff"], 1, '', qr/conversion .* failed:/);
check('verbose failure shows offending bytes', ['-v', '-f', 'UTF8', "\xff"],
      1, '', qr/from bytes \(1 bytes\): ff\nflags: 0x0000\nnetacnv: conversion .* failed:/);
check('maximum ASCII payload fits', ['-f', 'UTF8', 'a' x $maximum],
      0, 'from: ' . ('a' x $maximum) . "\nto: " . ('a' x $maximum) . "\n", '');
check('oversized input is rejected', ['a' x ($maximum + 1)], 1, '',
      qr/input is \d+ bytes; maximum is $maximum/);
check(
      'output expansion reports overflow',
      ['-f', 'MAC_ROMAN', "\x8e" x (int($maximum / 2) + 1)], 1, '',
      qr/conversion from 'MAC_ROMAN' to 'UTF8' failed:/
);

done_testing();
