package NadTest;

# Shared fixture for nad's Perl CLI tests.
# Copyright (C) 2026 Daniel Markstedt <daniel@mindani.net>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.

use strict;
use warnings;

use Cwd qw(getcwd realpath);
use File::Path qw(make_path);
use File::Temp qw(tempdir);
use IO::Select;
use IPC::Open3;
use Symbol qw(gensym);
use Test::More;

sub new {
    my ($class, $nad) = @_;
    BAIL_OUT('Usage: test script /path/to/nad') unless $nad;

    # Volume matching compares canonical paths; /var is a symlink on macOS.
    my $work = realpath(tempdir('nad-test-XXXXXX', TMPDIR => 1,
        CLEANUP => !$ENV{NAD_TEST_KEEP_DIR}));
    note("Keeping test files in $work") if $ENV{NAD_TEST_KEEP_DIR};

    my $self = bless {
        nad => $nad,
        work => $work,
        volume => "$work/volume",
        db => "$work/cnid",
        outside => "$work/outside",
        conf => "$work/afp.conf",
        seed_count => 0,
    }, $class;

    make_path($self->{volume}, $self->{db}, $self->{outside});
    my $ea = $ENV{NAD_TEST_EA} // 'sys';
    BAIL_OUT('NAD_TEST_EA must be sys or ad') unless $ea =~ /\A(?:sys|ad)\z/;
    open(my $config, '>', $self->{conf})
        or BAIL_OUT("Cannot write $self->{conf}: $!");
    print $config <<"CONF";
[Global]
log file = $work/netatalk.log

[nad_test]
path = $self->{volume}
cnid scheme = sqlite
vol dbpath = $self->{db}
ea = $ea
volume uuid = 11111111-1111-4111-8111-111111111111
CONF
    close $config or BAIL_OUT("Cannot close $self->{conf}: $!");
    return $self;
}

sub work { return $_[0]->{work}; }
sub volume { return $_[0]->{volume}; }
sub outside { return $_[0]->{outside}; }

# Detect the actual metadata backend, including ea=sys fallback on filesystems
# without native EAs. Importing MacBinary avoids the known mkdir/cp v2 crash.
sub uses_adouble_v2 {
    my ($self) = @_;
    return $self->{adouble_v2} if exists $self->{adouble_v2};
    my $probe = $self->seed_forked_file('backend_probe', '', '', 'TEXT', 'NADT');
    $self->{adouble_v2} = -d "$self->{volume}/.AppleDouble" ? 1 : 0;
    my $removed = $self->run('rm', $probe);
    BAIL_OUT("Cannot remove backend probe: $removed->{err}$removed->{out}")
        if $removed->{status} != 0;
    return $self->{adouble_v2};
}

# Capture both streams without a shell or a non-core Perl module.
sub _run {
    my ($self, $dir, @args) = @_;
    my $previous = getcwd();
    chdir $dir or BAIL_OUT("Cannot chdir to $dir: $!") if defined $dir;

    my ($in, $out);
    my $err = gensym;
    my $pid = eval {
        open3($in, $out, $err, $self->{nad}, '-F', $self->{conf}, @args)
    };
    my $open_error = $@;
    chdir $previous or BAIL_OUT("Cannot restore cwd $previous: $!")
        if defined $dir;
    BAIL_OUT("Cannot execute $self->{nad}: $open_error") if $open_error;
    close $in;

    my $out_fd = fileno($out);
    my $select = IO::Select->new($out, $err);
    my ($stdout, $stderr) = ('', '');
    while (my @ready = $select->can_read) {
        for my $fh (@ready) {
            my $n = sysread($fh, my $chunk, 8192);
            BAIL_OUT("Cannot read nad output: $!") unless defined $n;
            if ($n == 0) {
                $select->remove($fh);
                close $fh;
            } elsif (fileno($fh) == $out_fd) {
                $stdout .= $chunk;
            } else {
                $stderr .= $chunk;
            }
        }
    }
    waitpid($pid, 0);
    return { status => $?, out => $stdout, err => $stderr };
}

sub run {
    my ($self, @args) = @_;
    return $self->_run(undef, @args);
}

sub run_at {
    my ($self, $dir, @args) = @_;
    return $self->_run($dir, @args);
}

sub succeeds {
    my ($self, $label, @args) = @_;
    my $result = $self->run(@args);
    is($result->{status}, 0, $label)
        or diag("stdout: $result->{out}\nstderr: $result->{err}");
    return $result;
}

sub succeeds_at {
    my ($self, $label, $dir, @args) = @_;
    my $result = $self->run_at($dir, @args);
    is($result->{status}, 0, $label)
        or diag("stdout: $result->{out}\nstderr: $result->{err}");
    return $result;
}

sub file_contents {
    my ($self, $path) = @_;
    open(my $fh, '<:raw', $path) or return undef;
    my $contents = do { local $/; <$fh> };
    close $fh;
    return $contents;
}

sub write_file {
    my ($self, $path, $contents) = @_;
    open(my $fh, '>:raw', $path) or BAIL_OUT("Cannot write $path: $!");
    print $fh $contents;
    close $fh or BAIL_OUT("Cannot close $path: $!");
}

# Create a MacBinary III input independently of nad, then import its two forks.
sub seed_forked_file {
    my ($self, $name, $data, $resource, $type, $creator) = @_;
    BAIL_OUT('MacBinary fixture name must be 1..63 bytes')
        unless length($name) >= 1 && length($name) <= 63;
    BAIL_OUT('Finder type and creator must be four bytes each')
        unless length($type) == 4 && length($creator) == 4;

    my $header = "\0" x 128;
    substr($header, 1, 1) = pack('C', length($name));
    substr($header, 2, length($name)) = $name;
    substr($header, 65, 8) = $type . $creator;
    substr($header, 83, 8) = pack('NN', length($data), length($resource));
    my $mac_time = time() + 2_082_844_800;
    substr($header, 91, 8) = pack('NN', $mac_time, $mac_time);
    substr($header, 102, 4) = 'mBIN';
    substr($header, 122, 2) = pack('CC', 130, 129);

    my $crc = 0;
    for my $byte (unpack('C*', substr($header, 0, 124))) {
        $crc ^= $byte << 8;
        for (1 .. 8) {
            $crc = ($crc & 0x8000)
                ? (($crc << 1) ^ 0x1021) & 0xffff
                : ($crc << 1) & 0xffff;
        }
    }
    substr($header, 124, 2) = pack('n', $crc);

    my $pad_data = (128 - length($data) % 128) % 128;
    my $pad_resource = (128 - length($resource) % 128) % 128;
    my $path = sprintf('%s/seed-%d.bin', $self->{work}, ++$self->{seed_count});
    $self->write_file($path, $header . $data . ("\x7f" x $pad_data)
        . $resource . ("\x7f" x $pad_resource));

    my $result = $self->run_at($self->{volume}, 'unbin', $path);
    BAIL_OUT("Cannot seed $name: $result->{err}$result->{out}")
        if $result->{status} != 0;
    return "$self->{volume}/$name";
}

1;
