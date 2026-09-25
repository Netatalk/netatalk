#!/usr/bin/perl

# CLI integration tests for nad. No running AFP server is required.
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
my $outside = $fixture->outside;
my $adouble_v2 = $fixture->uses_adouble_v2;
# TODO: Re-enable mkdir/cp and dependent tests after fixing issue #3340, item 4.
# https://github.com/Netatalk/netatalk/issues/3340
my $v2_skip = 'AppleDouble v2 mkdir/cp crash (issue #3340, item 4)';

sub nad_cmd { return $fixture->run(@_); }
sub succeeds { return $fixture->succeeds(@_); }
sub file_contents { return $fixture->file_contents(@_); }

{
    note('volume boundary and forced local operations');
    my $path = "$outside/local_dir";
    my $rejected = nad_cmd('mkdir', $path);
    isnt($rejected->{status}, 0, 'path outside a volume is rejected');
    ok(!-e $path, 'rejected command creates nothing');
    succeeds('--force creates an ordinary directory', '--force', 'mkdir', $path);
    ok(-d $path, 'forced directory exists');
    succeeds('--force removes an ordinary directory', '--force', 'rmdir', $path);
    ok(!-e $path, 'forced directory was removed');
}

SKIP: {
    note('directory operations');
    skip $v2_skip, 11 if $adouble_v2;
    my $top = "$volume/nad_tree";
    my $leaf = "$top/child";
    succeeds('mkdir -p creates nested directories', 'mkdir', '-p', $leaf);
    ok(-d $leaf, 'nested directory exists');
    my $listed = succeeds('ls -ld reads the new directory', 'ls', '-l', '-d', $leaf);
    like($listed->{out}, qr/\bchild\b/, 'directory is listed');

    my $found = succeeds('find locates new directory in CNID',
        'find', '-v', $volume, 'child');
    is($found->{out}, "$leaf\n", 'find reports the expected path');

    my $protected = nad_cmd('rmdir', $volume);
    isnt($protected->{status}, 0, 'volume root cannot be removed');
    ok(-d $volume, 'volume root still exists');

    succeeds('rmdir -p removes nested directories', 'rmdir', '-p', $leaf);
    ok(!-e $top, 'directory tree was removed');
    my $gone = nad_cmd('find', '-v', $volume, 'child');
    isnt($gone->{status}, 0, 'removed directory is absent from CNID search');
}

SKIP: {
    note('copy, metadata, move, and remove');
    skip $v2_skip, 15 if $adouble_v2;
    my $source = "$outside/source.txt";
    my $first = "$volume/nad_first.txt";
    my $copy = "$volume/nad_copy.txt";
    my $moved = "$volume/nad_moved.txt";
    my $payload = "nad data fork\nsecond line\n";
    open(my $fh, '>:raw', $source) or BAIL_OUT("Cannot create $source: $!");
    print $fh $payload;
    close $fh;

    succeeds('copy from outside with --force', '--force', 'cp', $source, $first);
    is(file_contents($first), $payload, 'copied data fork matches');
    succeeds('set Finder type and creator', 'set', '-t', 'TEXT', '-c', 'NADT', $first);
    my $listed = succeeds('ls -l reads Finder metadata', 'ls', '-l', $first);
    like($listed->{out}, qr/\bTEXT NADT\b/, 'ls shows the new type and creator');

    succeeds('copy within the volume', 'cp', $first, $copy);
    is(file_contents($copy), $payload, 'second copy preserves data fork');
    # TODO: Re-enable when the cp Finder metadata bug is fixed in a future PR.
    # my $copy_list = succeeds('ls -l reads copied metadata', 'ls', '-l', $copy);
    # like($copy_list->{out}, qr/\bTEXT NADT\b/, 'second copy preserves Finder metadata');

    succeeds('move within the volume', 'mv', $copy, $moved);
    ok(!-e $copy, 'old pathname is gone');
    is(file_contents($moved), $payload, 'moved file preserves data fork');
    my $found = succeeds('find locates moved file in CNID',
        'find', '-v', $volume, 'nad_moved.txt');
    is($found->{out}, "$moved\n", 'CNID search reports new pathname');

    succeeds('remove moved file', 'rm', $moved);
    ok(!-e $moved, 'removed data fork is gone');
    my $gone = nad_cmd('find', '-v', $volume, 'nad_moved.txt');
    isnt($gone->{status}, 0, 'removed file is absent from CNID search');
}

SKIP: {
    note('listing and search modes');
    skip $v2_skip, 21 if $adouble_v2;
    my $dir = "$volume/list_modes";
    my $nested = "$dir/nested";
    succeeds('mkdir prepares listing tree', 'mkdir', '-p', $nested);
    $fixture->write_file("$outside/visible_seed", "visible\n");
    succeeds('cp registers a visible file in CNID', '--force', 'cp',
        "$outside/visible_seed", "$dir/visible");
    $fixture->write_file("$dir/.hidden", "hidden\n");
    $fixture->write_file("$nested/deep", "deep\n");

    my $plain = $fixture->succeeds_at('ls defaults to the current directory',
        $dir, 'ls');
    like($plain->{out}, qr/\bvisible\b/, 'default listing includes visible files');
    unlike($plain->{out}, qr/\.hidden/, 'default listing hides dotfiles');
    my $all = succeeds('ls -a includes dotfiles', 'ls', '-a', '-l', $dir);
    like($all->{out}, qr/\.hidden/, 'ls -a shows the hidden file');
    like($all->{out}, qr/\s\.\.?(?:\s|$)/m, 'ls -a shows dot entries');

    my $directory = succeeds('ls -d prints the directory itself',
        'ls', '-d', $dir);
    is($directory->{out}, "$dir\n", 'ls -d does not list directory contents');
    my $recursive = succeeds('ls -R descends into subdirectories',
        'ls', '-R', '-l', $dir);
    like($recursive->{out}, qr/\bdeep\b/, 'recursive listing reaches nested file');
    my $unix = succeeds('ls -lu includes Unix metadata',
        'ls', '-l', '-u', "$dir/visible");
    like($unix->{out}, qr/^-[rwxstST-]{9}\s/m,
        'ls -lu prints a Unix mode before Finder metadata');

    my $cwd_find = $fixture->succeeds_at('find searches the current volume',
        $volume, 'find', 'visible');
    is($cwd_find->{out}, "$dir/visible\n", 'find without -v reports the path');
    succeeds('cp registers another matching file in CNID',
        'cp', "$dir/visible", "$dir/visible_again");
    my $many = succeeds('find returns multiple CNID matches',
        'find', '-v', $volume, 'visible');
    is_deeply([sort split /\n/, $many->{out}],
        [sort "$dir/visible", "$dir/visible_again"],
        'find reports both matching paths');
    my $local = "$outside/local_match";
    $fixture->write_file($local, "local\n");
    my $local_find = $fixture->succeeds_at('forced find searches ordinary files',
        $outside, '--force', 'find', 'local_match');
    is($local_find->{out}, "$local\n", 'filesystem search finds the local file');
}

SKIP: {
    note('directory options and failures');
    skip $v2_skip, 13 if $adouble_v2;
    my $one = "$volume/verbose_one";
    my $two = "$volume/verbose_two";
    my $created = succeeds('mkdir -v accepts multiple directories',
        'mkdir', '-v', $one, $two);
    like($created->{out}, qr/\Q$one\E/, 'mkdir -v reports the first directory');
    like($created->{out}, qr/\Q$two\E/, 'mkdir -v reports the second directory');
    ok(-d $one && -d $two, 'both directories exist');
    isnt(nad_cmd('mkdir', $one)->{status}, 0,
        'mkdir without -p rejects an existing directory');
    succeeds('mkdir -p accepts an existing directory', 'mkdir', '-p', $one);
    $fixture->write_file("$one/child", "content\n");
    isnt(nad_cmd('rmdir', $one)->{status}, 0,
        'rmdir rejects a nonempty directory');
    ok(-d $one, 'failed rmdir leaves the directory');
    succeeds('rm clears the directory for rmdir', 'rm', "$one/child");
    my $removed = succeeds('rmdir -v accepts multiple directories',
        'rmdir', '-v', $one, $two);
    like($removed->{out}, qr/\Q$one\E/, 'rmdir -v reports the first directory');
    like($removed->{out}, qr/\Q$two\E/, 'rmdir -v reports the second directory');
    ok(!-e $one && !-e $two, 'both directories were removed');
}

{
    note('Finder labels, flags, and AFP attributes');
    my $file = $fixture->seed_forked_file('set_modes', "data\n", '',
        'TEXT', 'NADT');
    succeeds('set label, Finder flags, and AFP attributes',
        'set', '-l', 'red', '-f', 'DE', '-a', 'YP', $file);
    my $set = succeeds('ls reads changed file metadata', 'ls', '-l', $file);
    like($set->{out}, qr/\sde---------\s+y-p---\s+red\s/,
        'file flags, attributes, and label were set');
    succeeds('clear label, Finder flags, and AFP attributes',
        'set', '-l', 'none', '-f', 'de', '-a', 'yp', $file);
    my $cleared = succeeds('ls reads cleared file metadata', 'ls', '-l', $file);
    like($cleared->{out}, qr/\s-----------\s+------\s+---\s/,
        'file flags, attributes, and label were cleared');

    SKIP: {
        skip $v2_skip, 4 if $adouble_v2;
        my $dir = "$volume/set_directory";
        succeeds('mkdir prepares directory metadata test', 'mkdir', $dir);
        succeeds('set directory Finder flag and color label',
            'set', '-f', 'D', '-l', 'blue', $dir);
        my $dir_list = succeeds('ls reads directory metadata', 'ls', '-l', '-d', $dir);
        like($dir_list->{out}, qr/\sd----------\s+------\s+blu\s/,
            'set updates metadata on a directory');
    }
}

SKIP: {
    note('copy variants');
    skip $v2_skip, 33 if $adouble_v2;
    my $source = "$volume/copy_source";
    my $dest = "$volume/copy_dest";
    $fixture->write_file($source, "new\n");
    $fixture->write_file($dest, "old\n");
    succeeds('cp -n refuses to overwrite', 'cp', '-n', $source, $dest);
    is(file_contents($dest), "old\n", 'cp -n preserves destination contents');
    my $interactive = nad_cmd('cp', '-i', $source, $dest);
    isnt($interactive->{status}, 0, 'cp -i declines overwrite on EOF');
    like($interactive->{err}, qr/overwrite \Q$dest\E\?/, 'cp -i prompts before declining');
    is(file_contents($dest), "old\n", 'cp -i preserves destination contents');
    my $forced = succeeds('cp -fv overwrites with verbose output',
        'cp', '-f', '-v', $source, $dest);
    is(file_contents($dest), "new\n", 'cp -f replaces destination contents');
    like($forced->{out}, qr/\Q$source\E -> \Q$dest\E/,
        'cp -v reports the copied path');

    my $target_dir = "$volume/copy_targets";
    succeeds('mkdir prepares multi-source copy', 'mkdir', $target_dir);
    succeeds('cp copies multiple sources into a directory',
        'cp', $source, $dest, $target_dir);
    is(file_contents("$target_dir/copy_source"), "new\n",
        'first multi-source copy has expected contents');
    is(file_contents("$target_dir/copy_dest"), "new\n",
        'second multi-source copy has expected contents');

    my $tree = "$volume/copy_tree";
    succeeds('mkdir -p prepares recursive copy', 'mkdir', '-p', "$tree/nested");
    succeeds('cp places a file in the recursive source',
        'cp', $source, "$tree/nested/child");
    isnt(nad_cmd('cp', $tree, "$volume/no_recursive_copy")->{status}, 0,
        'cp refuses a directory without -R');
    my $tree_copy = "$volume/copy_tree_copy";
    succeeds('cp -R copies nested directories', 'cp', '-R', $tree, $tree_copy);
    is(file_contents("$tree_copy/nested/child"), "new\n",
        'recursive copy preserves nested data');
    my $copied_child = succeeds('find sees the recursively copied file',
        'find', '-v', $volume, 'child');
    like($copied_child->{out}, qr/^\Q$tree_copy\E\/nested\/child$/m,
        'recursive copy registers the child in CNID');

    my $preserved = "$volume/copy_preserved";
    chmod 0600, $source or BAIL_OUT("Cannot chmod $source: $!");
    my $timestamp = time() - 3600;
    utime $timestamp, $timestamp, $source
        or BAIL_OUT("Cannot set times on $source: $!");
    succeeds('cp -p preserves file mode and mtime', 'cp', '-p', $source, $preserved);
    is((stat($preserved))[2] & 0777, 0600, 'cp -p preserves mode');
    is((stat($preserved))[9], $timestamp, 'cp -p preserves modification time');
    my $archive_child = "$tree/nested/child";
    chmod 0640, $archive_child
        or BAIL_OUT("Cannot chmod $archive_child: $!");
    utime $timestamp, $timestamp, $archive_child
        or BAIL_OUT("Cannot set times on $archive_child: $!");
    succeeds('cp -a recursively preserves file mode and mtime',
        'cp', '-a', $tree, "$volume/copy_archive");
    my $archived_child = "$volume/copy_archive/nested/child";
    is(file_contents($archived_child), "new\n", 'cp -a copies a directory tree');
    is((stat($archived_child))[2] & 0777, 0640, 'cp -a preserves child mode');
    is((stat($archived_child))[9], $timestamp,
        'cp -a preserves child modification time');

    my $link = "$volume/copy_link";
    symlink 'copy_source', $link or BAIL_OUT("Cannot create $link: $!");
    succeeds('cp copies a symbolic link', 'cp', $link, "$volume/copied_link");
    ok(-l "$volume/copied_link", 'cp retains symbolic link type');
    is(readlink("$volume/copied_link"), 'copy_source',
        'cp retains symbolic link target');

    my $resource = "copy resource fork\n";
    my $forked = $fixture->seed_forked_file('copy_forked', "fork data\n",
        $resource, 'TEXT', 'NADT');
    my $fork_copy = "$volume/copied_forked";
    succeeds('cp copies a file with a resource fork', 'cp', $forked, $fork_copy);
    $fixture->succeeds_at('bin exports the copied resource fork',
        $volume, 'bin', '--filename', 'verify_copy', $fork_copy);
    my $binary = file_contents("$volume/verify_copy.bin");
    my $offset = 128 + int((length("fork data\n") + 127) / 128) * 128;
    is(unpack('N', substr($binary, 87, 4)), length($resource),
        'cp preserves resource fork length');
    is(substr($binary, $offset, length($resource)), $resource,
        'cp preserves resource fork bytes');
}

{
    note('move variants');
    my $source = "$volume/move_source";
    my $dest = "$volume/move_dest";
    $fixture->seed_forked_file('move_source', "move new\n", '',
        'TEXT', 'NADT');
    $fixture->seed_forked_file('move_dest', "move old\n", '',
        'TEXT', 'NADT');
    succeeds('mv -n refuses to overwrite', 'mv', '-n', $source, $dest);
    ok(-f $source, 'mv -n leaves source in place');
    is(file_contents($dest), "move old\n", 'mv -n preserves destination');
    my $interactive = succeeds('mv -i declines overwrite on EOF',
        'mv', '-i', $source, $dest);
    like($interactive->{err}, qr/overwrite \Q$dest\E\?/,
        'mv -i prompts before declining');
    ok(-f $source, 'mv -i leaves source in place');
    my $forced = succeeds('mv -fv overwrites and reports the move',
        'mv', '-f', '-v', $source, $dest);
    like($forced->{out}, qr/\Q$source\E -> \Q$dest\E/,
        'mv -v reports source and destination');
    ok(!-e $source, 'mv -f removes the source');
    is(file_contents($dest), "move new\n", 'mv -f replaces the destination');

    SKIP: {
        skip $v2_skip, 9 if $adouble_v2;
        my $dir = "$volume/move_directory";
        succeeds('mkdir prepares directory move', 'mkdir', $dir);
        $fixture->write_file("$dir/child", "directory move\n");
        succeeds('mv moves a directory', 'mv', $dir, "$volume/moved_directory");
        ok(!-e $dir, 'directory move removes old path');
        is(file_contents("$volume/moved_directory/child"), "directory move\n",
            'directory move retains nested contents');

        my $multi = "$volume/move_targets";
        succeeds('mkdir prepares multi-source move', 'mkdir', $multi);
        my $first = "$volume/move_first";
        my $second = "$volume/move_second";
        $fixture->seed_forked_file('move_first', "first\n", '',
            'TEXT', 'NADT');
        $fixture->seed_forked_file('move_second', "second\n", '',
            'TEXT', 'NADT');
        succeeds('mv moves multiple sources into a directory',
            'mv', $first, $second, $multi);
        is(file_contents("$multi/move_first"), "first\n", 'first source was moved');
        is(file_contents("$multi/move_second"), "second\n", 'second source was moved');
        ok(!-e $first && !-e $second, 'both old paths are gone');
    }

    my $resource = "move resource fork\n";
    my $forked = $fixture->seed_forked_file('move_forked', "fork data\n",
        $resource, 'TEXT', 'NADT');
    my $fork_dest = "$volume/moved_forked";
    succeeds('mv moves a file with a resource fork', 'mv', $forked, $fork_dest);
    $fixture->succeeds_at('bin exports the moved resource fork',
        $volume, 'bin', '--filename', 'verify_move', $fork_dest);
    my $binary = file_contents("$volume/verify_move.bin");
    my $offset = 128 + int((length("fork data\n") + 127) / 128) * 128;
    is(unpack('N', substr($binary, 87, 4)), length($resource),
        'mv preserves resource fork length');
    is(substr($binary, $offset, length($resource)), $resource,
        'mv preserves resource fork bytes');

    my $local = "$outside/moved_out";
    succeeds('forced mv copies a file out of an AFP volume',
        '--force', 'mv', $dest, $local);
    ok(!-e $dest, 'cross-boundary move removes volume source');
    is(file_contents($local), "move new\n", 'cross-boundary move keeps data');
    isnt(nad_cmd('find', '-v', $volume, 'move_dest')->{status}, 0,
        'cross-boundary move removes the source from CNID');
}

{
    note('recursive and verbose remove');
    SKIP: {
        skip $v2_skip, 7 if $adouble_v2;
        my $tree = "$volume/remove_tree";
        succeeds('mkdir -p prepares recursive removal',
            'mkdir', '-p', "$tree/nested");
        $fixture->write_file("$tree/nested/child", "remove me\n");
        SKIP: {
            # TODO: Issue #3340, item 1: rm without -R deletes child files.
            # Re-enable with assertions for nonzero status and untouched contents.
            skip 'rm without -R deletes directory contents (issue #3340, item 1)', 2;
            my $not_recursive = nad_cmd('rm', $tree);
            like($not_recursive->{out}, qr/\Q$tree\E\/nested is a directory/,
                'rm without -R reports the directory');
            ok(-d $tree, 'rm without -R leaves a directory intact');
        }
        my $removed = succeeds('rm -Rv removes a directory tree',
            'rm', '-R', '-v', $tree);
        ok(!-e $tree, 'rm -R removes the tree');
        like($removed->{out}, qr/\Q$tree\E/, 'rm -v reports removed paths');
        isnt(nad_cmd('find', '-v', $volume, 'remove_tree')->{status}, 0,
            'rm -R removes the directory from CNID');
    }

    my $link = "$volume/remove_link";
    symlink 'missing_target', $link or BAIL_OUT("Cannot create $link: $!");
    succeeds('rm removes a symbolic link', 'rm', $link);
    ok(!-l $link, 'rm removed the symbolic link');
}

done_testing;
