#!/usr/bin/env perl

# This script renders an afp_speedtest CSV capture into a throughput-vs-size
# chart (PNG) for publishing in CI, mirroring the flamegraph workflow.
# Rendering is delegated to gnuplot (gnuplot-nox is sufficient), so only
# core Perl modules are used.
#
# (c) 2026 Andy Lemin (@andylemin)
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

# The speedtest tool (test/testsuite/speedtest.c) emits, when run with -c,
# one "File Size Performance Summary" block per operation:
#
#     # File Size Performance Summary for Read
#     file_size_bytes,file_size_mb,mean_throughput_mbs,median_throughput_mbs,min_throughput_mbs,max_throughput_mbs,stddev_ms,mean_time_ms
#     4096,0.004,66.15,65.65,52.79,93.01,0.0,0.1
#     ...
#
# This parser keys on those headers and ignores any surrounding log noise
# (the container entrypoint prints setup messages on the same stdout), so
# the raw `docker run ... > speed.csv` capture can be fed in directly.
#
# Output: a single plot with X = file size (log2 scale) and Y = mean
# throughput (MB/s), one line per operation, with a shaded min..max band.
#
# Usage:
#     plot_speedtest.pl INPUT.csv [-o OUTPUT_BASENAME] [--title TITLE]
#                       [--subtitle SUBTITLE] [--baseline BASELINE.csv]
#                       [--meta KEY=VALUE ...]
#
# Writes OUTPUT_BASENAME.png (default basename: "speedtest").

use strict;
use warnings;
use Getopt::Long qw(GetOptions);

# Operations in the order the speedtest runs them, with stable colours so
# the chart reads the same across commits.
my @OPERATIONS = qw(Read Write Copy ServerCopy);
my %OP_COLORS = (
                 Read       => '#1f77b4',    # blue
                 Write      => '#d62728',    # red
                 Copy       => '#2ca02c',    # green
                 ServerCopy => '#ff7f0e',    # orange
);
my $DEFAULT_COLOR = '#7f7f7f';               # any unexpected operation

# Near-white canvas (RGB 246,248,250) for a clean modern background.
my $BACKGROUND_COLOR = '#F6F8FA';

# 11 x 5.2 inches at 130 dpi, matching the former matplotlib renderer.
my $WIDTH_PX  = 1430;
my $HEIGHT_PX = 676;

# Column header line that immediately precedes the data rows.
my $COLUMN_HEADER_PREFIX = 'file_size_bytes,';

# Parse a speedtest CSV capture. Returns (results, meta) where:
#   * results = {operation => [row_hash, ...]}, each row_hash with keys
#     size_bytes, mean, median, min, max; rows sorted by ascending size.
#   * meta = hash of run metadata derived from the capture: "iterations"
#     (max per-iteration index seen) plus any key=value pairs from an
#     optional "# Config: k=v,k=v" line emitted by the tool.
sub parse_csv {
    my ($path) = @_;
    my (%results, %meta);
    my $max_iter   = 0;
    my $current_op = undef;
    my $in_data    = 0;

    open my $fh, '<', $path or die "cannot open $path: $!\n";
    while (my $line = <$fh>) {
        $line =~ s/^\s+|\s+$//g;

        # Optional self-describing config line, e.g.
        # "# Config: quantum_kb=1024,iterations=5,warmup=1,afp=3.4".
        if ($line =~ /#\s*Config:\s*(.+)/i) {
            for my $pair (split /,/, $1) {
                next unless index($pair, '=') >= 0;
                my ($k, $v) = split /=/, $pair, 2;
                s/^\s+|\s+$//g for ($k, $v);
                $meta{$k} = $v;
            }
            next;
        }

        # Per-iteration row: "Op,iteration,file_size_mb,microseconds,tput".
        if ($line =~ /^[A-Za-z]+,(\d+),[\d.]+,\d+,[\d.]+\s*$/) {
            $max_iter = $1 if $1 > $max_iter;
        }

        if ($line =~ /#\s*File Size Performance Summary for\s+(\S+)/i) {
            $current_op = $1;
            $results{$current_op} ||= [];
            $in_data = 0;
            next;
        }
        next unless defined $current_op;

        if (index($line, $COLUMN_HEADER_PREFIX) == 0) {
            $in_data = 1;
            next;
        }
        next unless $in_data;

        # Summary row: size_bytes,size_mb,mean,median,min,max,stddev,time.
        my @parts = split /,/, $line;
        if (@parts >= 6
             && !grep { !/^-?[\d.]+$/ } @parts[0, 2 .. 5]) {
            push @{$results{$current_op}}, {
                                            size_bytes => $parts[0] + 0,
                                            mean       => $parts[2] + 0,
                                            median     => $parts[3] + 0,
                                            min        => $parts[4] + 0,
                                            max        => $parts[5] + 0,
            };
        } else {
            $in_data = 0;    # blank line / next section / log noise
        }
    }
    close $fh;

    for my $op (keys %results) {
        if (!@{$results{$op}}) {
            delete $results{$op};
            next;
        }
        $results{$op} = [sort { $a->{size_bytes} <=> $b->{size_bytes} } @{$results{$op}}];
    }
    $meta{iterations} = $max_iter if $max_iter && !$meta{iterations};
    return (\%results, \%meta);
}

# Format an exact byte count as an integer KiB/MiB axis label.
sub format_size {
    my ($size_bytes) = @_;
    my $kib = $size_bytes / 1024.0;
    return sprintf('%dK', $kib + 0.5) if $kib < 1024.0;
    return sprintf('%dM', $kib / 1024.0 + 0.5);
}

# Known operations first (stable colour/order), then any unexpected ones.
sub ordered_ops {
    my ($results) = @_;
    my @ops = grep { $results->{$_} } @OPERATIONS;
    push @ops, grep {
        my $op = $_;
        !grep { $_ eq $op } @OPERATIONS
    } sort keys %$results;
    return @ops;
}

# Compose the run-config lines shown in the chart's info box. Values come
# from the CSV where available (size range, iterations) and from --meta
# key=value pairs the caller injects for things the CSV cannot carry
# (quantum, ARC dircache, AFP version). Missing values are omitted rather
# than guessed.
sub build_info_lines {
    my ($results, $meta) = @_;
    my @ops = ordered_ops($results);

    # Size sweep is omitted here — the X axis already shows the range.
    my @lines;
    if ($meta->{iterations}) {
        my $warm = $meta->{warmup} ? " +$meta->{warmup} warmup" : '';
        push @lines, "Iterations: $meta->{iterations}$warm";
    }
    push @lines, 'Ops: ' . join(', ', @ops);
    push @lines, "AFP: $meta->{afp}"               if $meta->{afp};
    push @lines, "Quantum: $meta->{quantum_kb} KB" if $meta->{quantum_kb};
    my @dc;
    push @dc, uc $meta->{dircache_mode}     if $meta->{dircache_mode};
    push @dc, "size $meta->{dircache_size}" if $meta->{dircache_size};
    push @dc, "freq $meta->{dircache_validation_freq}"
      if $meta->{dircache_validation_freq};
    push @lines, 'Dircache: ' . join(' · ', @dc) if @dc;
    return @lines;
}

# Escape a string for embedding in a double-quoted gnuplot string.
# Literal newlines become \n escapes (gnuplot renders them as line breaks).
sub gp_quote {
    my ($s) = @_;
    $s =~ s/\\/\\\\/g;
    $s =~ s/"/\\"/g;
    $s =~ s/\n/\\n/g;
    return $s;
}

# Render the throughput-vs-size chart to <out_base>.png via gnuplot.
sub plot {
    my ($results, $out_base, $title, $subtitle, $baseline, $meta) = @_;
    my @ordered = ordered_ops($results);
    if (!@ordered) {
        print STDERR "ERROR: no speedtest summary data found in input — " . "was afp_speedtest run with -c (CSV)?\n";
        return 0;
    }

    # Collect plot commands into separate z-layers (see @plots below) so a
    # later op's fill or line never paints over an earlier op's curve. Draw
    # order then no longer matches legend order, so every data plot is
    # notitle and the legend is rebuilt from keyentry items in per-op order.
    my $blocks = '';
    my (
         @bands, @means, @baselines, @meancurves, @maxes, @keyentries,
         @labels
    );
    my $idx = 0;
    for my $op (@ordered) {
        my $rows  = $results->{$op};
        my $color = $OP_COLORS{$op} || $DEFAULT_COLOR;
        my $tag   = "OP$idx";
        $blocks .= "\$$tag << EOD\n";
        for my $r (@$rows) {
            $blocks .= "$r->{size_bytes} $r->{mean} $r->{min} $r->{max}\n";
        }
        $blocks .= "EOD\n";

        # min..max band gives a sense of run-to-run variance.
        push @bands,
          "\$$tag using 1:3:4 with filledcurves "
          . "fillcolor rgb \"$color\" fillstyle transparent solid 0.12 "
          . 'notitle';
        # Mean throughput line.
        push @meancurves,
          "\$$tag using 1:2 with linespoints linewidth 2 pointtype 7 "
          . "pointsize 0.5 linecolor rgb \"$color\" notitle";
        # Max (best run) traces the top of the band, drawn as a dotted line
        # in the op colour but alpha-blended (#80… = 50% transparent) so
        # it reads lighter than the solid mean.
        my $light = '#80' . substr($color, 1);
        push @maxes,
          "\$$tag using 1:4 with lines dashtype (2,1) linewidth 1 " . "linecolor rgb \"$light\" notitle";

        # Two horizontal references per op: the mean of the per-size mean
        # series as a solid line (matching the solid mean curve) and the mean
        # of the max series as a dotted line (matching the dotted max hats).
        # Value labels sit just inside the plot edges so they never overlap
        # the Y ticks.
        my ($sum_mean, $sum_max) = (0, 0);
        for my $r (@$rows) {
            $sum_mean += $r->{mean};
            $sum_max  += $r->{max};
        }
        my $avg     = $sum_mean / scalar(@$rows);
        my $avg_max = $sum_max / scalar(@$rows);
        push @means,
          sprintf(
                  '%s with lines linewidth 1 linecolor rgb "%s" notitle',
                  $avg, $color
          );
        push @means,
          sprintf(
                  '%s with lines dashtype (2,1) linewidth 1 linecolor rgb "%s" ' . 'notitle',
                  $avg_max, $light
          );
        # "boxed" gives the matplotlib-style white backing behind the value
        # so it stays readable over the grid (see set style textbox). The
        # avg-mean label is left-anchored at the Y axis; the avg-max label
        # is right-aligned (the right edge) so the two never collide.
        push @labels,
          sprintf(
                    'set label %d "%.0f" at graph 0.012, first %s left front '
                  . 'offset 0,0.5 boxed font ",5" textcolor rgb "%s"',
                  $idx + 10, $avg, $avg, $color
          );
        push @labels,
          sprintf(
                    'set label %d "%.0f" at graph 0.988, first %s right front '
                  . 'offset 0,0.5 boxed font ",5" textcolor rgb "%s"',
                  $idx + 30, $avg_max, $avg_max, $color
          );

        # Legend: only the per-op mean curve (the avg-max/quantum overlays are
        # explained by the on-chart labels, keeping the key compact).
        push @keyentries,
          sprintf(
                    'keyentry with linespoints linewidth 2 pointtype 7 '
                  . 'pointsize 0.5 linecolor rgb "%s" title "%s (mean)"',
                  $color, gp_quote($op)
          );

        # Mark the dircache quantum (1 MB) on the Read curve: label the
        # data point at exactly 1 MiB so the rfork-cache boundary is
        # visible on the chart.
        if ($op eq 'Read') {
            for my $r (@$rows) {
                next unless $r->{size_bytes} == 1024 * 1024;
                push @labels,
                  sprintf(
                            'set label 9 "quantum" at first %s, first %s '
                          . 'offset 0.6,0.8 front font ",5" '
                          . "textcolor rgb \"$color\"",
                          $r->{size_bytes}, $r->{mean}
                  );
                last;
            }
        }

        if ($baseline && $baseline->{$op} && @{$baseline->{$op}}) {
            my $btag = "BASE$idx";
            $blocks .= "\$$btag << EOD\n";
            for my $r (@{$baseline->{$op}}) {
                $blocks .= "$r->{size_bytes} $r->{mean}\n";
            }
            $blocks .= "EOD\n";
            push @baselines,
              "\$$btag using 1:2 with lines dashtype (4,2) linewidth 1.3 " . "linecolor rgb \"$color\" notitle";
            push @keyentries,
              "keyentry with lines dashtype (4,2) linewidth 1.3 "
              . "linecolor rgb \"$color\" "
              . "title \"@{[ gp_quote($op) ]} (baseline)\"";
        }
        $idx++;
    }
    # Bottom-to-top: bands then their max "hats" (the band top edge), baseline
    # overlays, mean curves, then the avg reference lines on top so they
    # stay visible across the data, then the (non-drawing) legend entries.
    my @plots = (
                 @bands, @maxes, @baselines, @meancurves, @means,
                 @keyentries
    );

    # Log2 X axis with one integer KiB/MiB tick per swept size, clamped.
    my %all_sizes;
    for my $rows (values %$results) {
        for my $r (@$rows) {
            $all_sizes{$r->{size_bytes}} = 1;
        }
    }
    my @all_sizes = sort { $a <=> $b } keys %all_sizes;
    my @tic_specs = map  { sprintf '"%s" %s', format_size($_), $_ } @all_sizes;
    my $tics      = join(', ', @tic_specs);

    my @header;
    push @header, $subtitle if defined $subtitle && $subtitle ne '';
    my @info = build_info_lines($results, $meta);
    push @header, join(' · ', @info) if @info;
    my $header = gp_quote(join("\n", @header));

    my $suptitle = gp_quote($title || 'Netatalk AFP Speedtest — throughput vs file size');
    my $png      = "$out_base.png";
    my $labels   = join("\n",          @labels);
    my $plots    = join(", \\\n     ", @plots);

    my $gp_bin = $ENV{GNUPLOT_BIN} || 'gnuplot';
    open my $gp, '|-', $gp_bin or die "cannot run $gp_bin: $!\n";
    print $gp <<"EOF";
# DejaVu Sans is matplotlib's default face; pangocairo falls back to the
# system sans when it is absent, so this renders everywhere. The 1.8
# fontscale/linewidth/pointscale matches matplotlib's dpi=130 rendering
# (130/72), so point sizes below mirror the old matplotlib values.
set terminal pngcairo noenhanced size $WIDTH_PX,$HEIGHT_PX \\
    fontscale 1.8 linewidth 1.8 pointscale 1.8 \\
    background rgb "$BACKGROUND_COLOR" font "DejaVu Sans,7"
set output "@{[ gp_quote($png) ]}"

# Full matplotlib-style frame: box border, ticks outward on left/bottom.
set border 15 linewidth 0.8 linecolor rgb "#333333"
set tics nomirror out scale 0.6

# Reserve the top band for the manually placed title block (a fixed gap
# between the bold title and the subtitle, like the matplotlib layout).
set tmargin at screen 0.84
set bmargin at screen 0.12
set label 1 "$suptitle" at screen 0.5, screen 0.95 center \\
    font "DejaVu Sans:Bold,11"
set label 2 "$header" at screen 0.5, screen 0.885 center \\
    font "DejaVu Sans,5.5" textcolor rgb "#222222"

set logscale x 2
# Tiny right-margin factor so the boundary tick label is not clipped.
set xrange [$all_sizes[0]:@{[ $all_sizes[-1] * 1.001 ]}]
set xtics font ",7" ($tics)
set xlabel "File size"
set ylabel "Throughput (MB/s)"
# Fine dotted grid matching the lantest chart. Major lines at #BBBBBB;
# minor lines a touch fainter so they don't read heavy.
set grid xtics ytics mxtics mytics linetype 1 dashtype (1,2) linewidth 1 \\
    linecolor rgb "#BBBBBB", linetype 1 dashtype (1,2) linewidth 1 \\
    linecolor rgb "#DDDDDD"
# Legend pinned to the free top-left of the outer canvas (above the plot,
# left of the centred title/subtitle) so it sits entirely outside the plot
# area and never overlaps a data curve.
set key at screen 0.005, screen 0.995 top left Left reverse samplen 1.5 \\
    width -1 spacing 1.1 font ",6" \\
    box linewidth 0.6 linecolor rgb "#CCCCCC" opaque fillcolor rgb "#F5F5F5"
# White backing for the boxed mean-value labels (matplotlib bbox style).
set style textbox opaque fillcolor rgb "#FFFFFF" noborder margins 0.6,0.6
# Column headers over the two value-label stacks. Each value is a mean
# across the size sweep: "avg mean" at the left (Y axis), "avg max" at
# the right edge.
set label 3 "avg mean" at graph 0.012, graph 1.02 left front \\
    font "DejaVu Sans:Bold,6" textcolor rgb "#444444"
set label 4 "avg max" at graph 0.988, graph 1.02 right front \\
    font "DejaVu Sans:Bold,6" textcolor rgb "#444444"
$labels

$blocks
plot $plots
EOF
    close $gp or die "gnuplot failed (status $?)\n";
    print STDERR "Wrote $png\n";
    return 1;
}

sub main {
    my $output   = 'speedtest';
    my $title    = undef;
    my $subtitle = undef;
    my $baseline_path;
    my @meta_args;
    GetOptions(
               'o|output=s' => \$output,
               'title=s'    => \$title,
               'subtitle=s' => \$subtitle,
               'baseline=s' => \$baseline_path,
               'meta=s'     => \@meta_args,
      )
      or die "usage: plot_speedtest.pl INPUT.csv [-o BASENAME] "
      . "[--title T] [--subtitle S] [--baseline B.csv] "
      . "[--meta KEY=VALUE ...]\n";
    my $input = shift @ARGV
      or die "missing input CSV (afp_speedtest -c capture)\n";

    my ($results, $meta) = parse_csv($input);
    # CLI --meta overrides anything parsed from the CSV.
    for my $pair (@meta_args) {
        next unless index($pair, '=') >= 0;
        my ($k, $v) = split /=/, $pair, 2;
        s/^\s+|\s+$//g for ($k, $v);
        $meta->{$k} = $v;
    }

    my $baseline;
    if ($baseline_path) {
        if (-r $baseline_path) {
            ($baseline) = parse_csv($baseline_path);
        } else {
            print STDERR "WARNING: could not read baseline: $baseline_path\n";
        }
    }

    return plot($results, $output, $title, $subtitle, $baseline, $meta)
      ? 0
      : 1;
}

exit main();
