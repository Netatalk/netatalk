#!/usr/bin/env perl

# This script generates a compilation readme from GitHub workflow files
#
# (c) 2025-2026 Daniel Markstedt <daniel@mindani.net>
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
use YAML::PP;
use YAML::PP::Common qw/ PRESERVE_ORDER /;

if (grep { $_ eq '--help' || $_ eq '-h' } @ARGV) {
    print "Usage: $0 input_file [input_file ...] output_file\n";
    print "  input_file   Path to a workflow YAML file (one or more required)\n";
    print "  output_file  Path to the output markdown file (required)\n";
    exit 0;
}

if (@ARGV < 2) {
    print "Usage: $0 input_file [input_file ...] output_file\n";
    print "  input_file   Path to a workflow YAML file (one or more required)\n";
    print "  output_file  Path to the output markdown file (required)\n";
    exit 1;
}

my $output_file = pop @ARGV;
my @input_files = @ARGV;

foreach my $input_file (@input_files) {
    unless (-e $input_file) {
        die "Error: Input file '$input_file' does not exist.\n";
    }
}

my $ypp = YAML::PP->new(preserve => PRESERVE_ORDER);

sub format_shell_command {
    my ($shell) = @_;

    return unless defined $shell;

    $shell =~ s/\\\n\s+//g;
    $shell =~ s/-Dbuildtype=debugoptimized/-Dbuildtype=release/g;
    $shell =~ s/-Dwith-init-hooks=false/-Dwith-init-hooks=true/g;
    $shell =~ s/^\s*set\b[^\n]*(?:\n|$)//gm;
    $shell =~ s/^\s*.*-V\s*(?:\n|$)//gm;
    $shell =~ s/\n+$//;

    return $shell;
}

sub wanted_step {
    my ($step) = @_;

    return unless exists $step->{name};

    my %wanted = map { $_ => 1 } (
                                  'Install dependencies',
                                  'Configure',
                                  'Build',
                                  'Test',
                                  'Install',
                                  'Build on VM',
    );

    return $wanted{$step->{name}};
}

sub format_job_name {
    my ($name) = @_;

    if ($name =~ /-\s*([^(]+?)\s*\(/) {
        return $1;
    }

    return $name;
}

sub render_step {
    my ($markdown, $step) = @_;

    if (exists $step->{uses} && $step->{uses} =~ /^vmactions\//) {
        push @{$markdown},
          "Install dependencies",
          "",
          "```shell",
          format_shell_command($step->{with}->{prepare}),
          "```",
          "",
          "Build and install",
          "",
          "```shell",
          format_shell_command($step->{with}->{run}),
          "```",
          "";
    } else {
        push @{$markdown},
          $step->{name},
          "",
          "```shell",
          format_shell_command($step->{run}),
          "```",
          "";
    }
}

my @markdown = (
            "# Compile Netatalk from Source",
            "",
            "Below are instructions on how to compile Netatalk from source for specific operating systems.",
            "Before starting, please read through the [Install Quick Start](https://netatalk.io/install) guide first.",
            "You need to have a local clone of Netatalk's source code before proceeding.",
            "",
            "```shell",
            "git clone https://github.com/Netatalk/netatalk.git",
            "cd netatalk",
            "```",
            "",
            "**Note:** Installation commands may require `sudo` privileges depending on your system configuration.",
            "",
);

foreach my $input_file (@input_files) {
    my $workflow;
    eval { $workflow = $ypp->load_file($input_file); };
    if ($@) {
        die "Error parsing YAML file '$input_file': $@\n";
    }

    foreach my $key (keys %{$workflow->{jobs}}) {
        my $job = $workflow->{jobs}->{$key};

        next if $job->{name} =~ /\(32-bit\)$/;

        my @steps = grep { wanted_step($_) } @{$job->{steps}};
        next unless @steps;

        push @markdown, "## " . format_job_name($job->{name}), "";

        foreach my $step (@steps) {
            render_step(\@markdown, $step);
        }
    }
}

open(my $fh, '>', $output_file) or die "Could not open file '$output_file' for writing: $!";
print $fh join("\n", @markdown);
close $fh;

print "Wrote to $output_file\n";
