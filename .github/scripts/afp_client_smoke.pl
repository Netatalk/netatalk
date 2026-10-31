#!/usr/bin/env perl

# Exercise file operations on an already mounted AFP volume
#
# Copyright (c) 2026 Daniel Markstedt <daniel@mindani.net>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

use strict;
use warnings;
use utf8;
use Cwd            qw(abs_path);
use Encode         qw(decode_utf8 encode_utf8);
use File::Basename qw(dirname);
use File::Copy     qw(copy);
use File::Path     qw(make_path);
use File::Temp;
use IO::Handle;
use Test::More;
use Unicode::Normalize qw(NFC);

sub write_file {
    my ($path, $data) = @_;
    open my $fh, '>:raw', $path or die "open $path: $!";
    print {$fh} $data or die "write $path: $!";
    close $fh         or die "close $path: $!";
}

sub read_file {
    my ($path) = @_;
    open my $fh, '<:raw', $path or die "open $path: $!";
    local $/;
    my $data = <$fh>;
    die "read $path: $!" unless defined $data;
    close $fh or die "close $path: $!";
    return $data;
}

sub entries {
    my ($path) = @_;
    opendir my $dh, $path or die "opendir $path: $!";
    my @names = sort grep { $_ ne '.' && $_ ne '..' } readdir $dh;
    closedir $dh or die "closedir $path: $!";
    return \@names;
}

sub xattr {
    open my $fh, '-|', '/usr/bin/xattr', @_ or die "start xattr: $!";
    local $/;
    my $output = <$fh> // '';
    close $fh or die "xattr failed (status $?)";
    $output =~ s/\n\z//;
    return $output;
}

sub run_tests {
    my ($mountpoint) = @_;

    subtest 'create, read and append' => sub {
        my $dir  = File::Temp->newdir('smoke-XXXXXX', DIR => $mountpoint);
        my $path = "$dir/file.txt";
        write_file($path, "hello\n");
        open my $fh, '>>:raw', $path or die "open $path: $!";
        print {$fh} "AFP\n" or die "append $path: $!";
        close $fh           or die "close $path: $!";
        is(read_file($path), "hello\nAFP\n", 'appended contents');
        is(-s $path,         10,             'file size');
    };

    subtest 'seek and truncate' => sub {
        my $dir  = File::Temp->newdir('smoke-XXXXXX', DIR => $mountpoint);
        my $path = "$dir/random-access.bin";
        write_file($path, '0123456789');
        open my $fh, '+<:raw', $path or die "open $path: $!";
        seek($fh, 3, 0)   or die "seek $path: $!";
        print {$fh} 'AFP' or die "write $path: $!";
        # Flush buffered writes before truncating the file.
        $fh->flush       or die "flush $path: $!";
        truncate($fh, 8) or die "truncate $path: $!";
        close $fh        or die "close $path: $!";
        is(read_file($path), '012AFP67', 'updated and truncated contents');
    };

    subtest 'binary copy' => sub {
        my $dir    = File::Temp->newdir('smoke-XXXXXX', DIR => $mountpoint);
        my $data   = pack('C*', 0 .. 255) x 4096;
        my $source = "$dir/source.bin";
        my $target = "$dir/copy.bin";
        write_file($source, $data);
        copy($source, $target) or die "copy: $!";
        is(read_file($target), $data, 'copied binary contents');
        unlink $source or die "unlink $source: $!";
        is(read_file($target), $data, 'copy survives source deletion');
    };

    subtest 'rename, move and replace' => sub {
        my $dir = File::Temp->newdir('smoke-XXXXXX', DIR => $mountpoint);
        mkdir "$dir/subdir" or die "mkdir: $!";
        my $source  = "$dir/original.txt";
        my $renamed = "$dir/renamed.txt";
        my $target  = "$dir/subdir/target.txt";
        write_file($source, 'original');
        rename $source, $renamed or die "rename: $!";
        ok(!-e $source, 'original name removed');
        write_file($target, 'replace me');
        rename $renamed, $target or die "replace: $!";
        ok(!-e $renamed, 'renamed file moved');
        is(read_file($target), 'original', 'destination replaced');
    };

    subtest 'directories, listing and deletion' => sub {
        my $dir    = File::Temp->newdir('smoke-XXXXXX', DIR => $mountpoint);
        my $parent = "$dir/parent";
        my $child  = "$parent/child";
        my $path   = "$child/empty";
        make_path($child);
        write_file($path, '');
        is_deeply(entries($child), ['empty'], 'directory listing');
        is(-s $path, 0, 'empty file size');
        unlink $path  or die "unlink $path: $!";
        rmdir $child  or die "rmdir $child: $!";
        rmdir $parent or die "rmdir $parent: $!";
        ok(!-e $parent, 'directories deleted');
    };

    subtest 'Unicode and spaces' => sub {
        my $dir       = File::Temp->newdir('smoke-XXXXXX', DIR => $mountpoint);
        my $directory = "$dir/" . encode_utf8('Données 日本語');
        my $path      = "$directory/" . encode_utf8('smörgås 🍎.txt');
        my $data      = encode_utf8("Héllo 日本語\n");
        mkdir $directory or die "mkdir: $!";
        write_file($path, $data);
        # macOS may normalize filenames, so look up the original spelling.
        ok(-e $path, 'original spelling resolves');
        is(read_file($path), $data, 'UTF-8 contents');
        my $listed = entries($directory);
        is(scalar @$listed,                      1,                    'one directory entry');
        is(NFC(decode_utf8($listed->[0] // '')), NFC('smörgås 🍎.txt'), 'listed name matches');
    };

    subtest 'extended attributes' => sub {
        my $dir  = File::Temp->newdir('smoke-XXXXXX', DIR => $mountpoint);
        my $path = "$dir/attributes.txt";
        my $name = 'org.netatalk.smoke';
        write_file($path, '');
        xattr('-w', $name, 'AFP attribute', $path);
        my @names = split /\n/, xattr($path);
        ok((grep { $_ eq $name } @names), 'attribute listed');
        is(xattr('-p', $name, $path), 'AFP attribute', 'attribute value');
        xattr('-w', $name, 'updated', $path);
        is(xattr('-p', $name, $path), 'updated', 'attribute updated');
        xattr('-d', $name, $path);
        @names = split /\n/, xattr($path);
        ok(!(grep { $_ eq $name } @names), 'attribute removed');
    };

    subtest 'resource fork' => sub {
        my $dir     = File::Temp->newdir('smoke-XXXXXX', DIR => $mountpoint);
        my $path    = "$dir/forks.bin";
        my $renamed = "$dir/renamed-forks.bin";
        write_file($path,                    'data fork');
        write_file("$path/..namedfork/rsrc", 'resource fork');
        is(read_file($path),                    'data fork',     'data fork contents');
        is(read_file("$path/..namedfork/rsrc"), 'resource fork', 'resource fork contents');
        rename $path, $renamed or die "rename: $!";
        is(read_file("$renamed/..namedfork/rsrc"), 'resource fork', 'resource fork survives rename');
    };

    done_testing();
}

sub main {
    @ARGV == 1 or die "Usage: $0 MOUNTPOINT\n";
    my $mountpoint = abs_path($ARGV[0]);
    defined $mountpoint && -d $mountpoint or die "Not a directory: $ARGV[0]\n";
    my @stat   = stat $mountpoint;
    my @parent = stat dirname($mountpoint);
    @stat && @parent or die "Cannot stat mountpoint: $!";
    ($stat[0] != $parent[0] || $stat[1] == $parent[1])
      or die "Not a mounted volume: $mountpoint\n";
    run_tests($mountpoint);
}

main() unless caller;

1;
