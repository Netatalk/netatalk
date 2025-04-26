#!/usr/bin/env perl

# This script generates the compilation readme from the GitHub build.yml file.
#
# (c) 2025 Daniel Markstedt <daniel@mindani.net>
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
use YAML::PP::Common qw/ :PRESERVE /;

if (@ARGV < 2 || grep { $_ eq '--help' || $_ eq '-h' } @ARGV) {
    print "Usage: $0 input_file output_file\n";
    print "  input_file   Path to the build.yml file (required)\n";
    print "  output_file  Path to the output markdown file (required)\n";
    exit((@ARGV < 2) ? 1 : 0);
}

my $input_file = $ARGV[0];
my $output_file = $ARGV[1];

unless (-e $input_file) {
    die "Error: Input file '$input_file' does not exist.\n";
}

my $ypp = YAML::PP->new(preserve => PRESERVE_ORDER);
my $workflow;
eval {
    $workflow = $ypp->load_file($input_file);
};
if ($@) {
    die "Error parsing YAML file: $@\n";
}

my @markdown = (
    "# Compile Netatalk from Source",
    "",
    "Below are instructions on how to compile Netatalk from source for specific operating systems.",
    "Before starting, please read through the [Install Quick Start](https://netatalk.io/install) guide first.",
    "You need to have a local clone of Netatalk's source code before proceeding.",
    "",
    "Please note that these steps are automatically generated from the CI jobs,",
    "and may not always be optimized for standalone execution.",
    "",
);

foreach my $key (keys %{$workflow->{jobs}}) {
    my $job = $workflow->{jobs}->{$key};

    # No need to document the SonarQube job
    next if $job->{name} eq "Static Analysis";

    push @markdown, "## " . $job->{name}, "";

    foreach my $step (@{$job->{steps}}) {
        # Skip unnamed steps
        next unless exists $step->{name};

        # Skip GitHub actions steps irrelevant to documentation
        next if exists $step->{uses} && $step->{uses} =~ /^actions\//;

        # Adjust line breaks for better readability
        if (exists $step->{run}) {
            $step->{run} =~ s/\\\n\s+//g;
            $step->{run} =~ s/\n+$//;
        }

        if (exists $step->{with}) {
            if (exists $step->{with}->{prepare}) {
                $step->{with}->{prepare} =~ s/\\\n\s+//g;
                $step->{with}->{prepare} =~ s/\n+$//;
            }

            if (exists $step->{with}->{run}) {
                $step->{with}->{run} =~ s/\\\n\s+//g;
                $step->{with}->{run} =~ s/\n+$//;
            }
        }

        # There are two types of jobs with different structures
        if (exists $step->{uses} && $step->{uses} =~ /^vmactions\//) {
            push @markdown, 
                "Install required packages",
                "",
                "```",
                $step->{with}->{prepare},
                "```",
                "",
                "Configure, compile, install, run, and uninstall",
                "",
                "```",
                $step->{with}->{run},
                "```",
                "";
        } else {
            push @markdown,
                $step->{name},
                "",
                "```",
                $step->{run},
                "```",
                "";
        }
    }
}

open(my $fh, '>', $output_file) or die "Could not open file '$output_file' for writing: $!";
print $fh join("\n", @markdown);
close $fh;

print "Wrote to $output_file\n";
