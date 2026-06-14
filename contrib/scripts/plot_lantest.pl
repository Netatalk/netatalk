#!/usr/bin/env perl

# This script renders an afp_lantest CSV capture into a per-test latency
# candlestick chart (PNG) for publishing in CI, mirroring the speedtest
# perfgraph workflow. Rendering is delegated to gnuplot (gnuplot-nox is
# sufficient), so only core Perl modules are used.
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

# afp_lantest, run with -c, prints one summary row per test:
#
#     Test,Time_ms,Time±,Min_ms,Max_ms,Median_ms,MB/s
#     Open, stat and read 512 bytes from 1000 files [8,000 AFP ops],360,33.6,324,406,349,0
#     ...
#     Sum of all AFP OPs = 80686,4921,,
#
# Two parsing wrinkles handled here:
#   * test names contain commas, so rows are parsed right-anchored — the
#     numeric columns are the trailing fields, the name is everything
#     before them.
#   * a trailing "Sum of all AFP OPs" row is ignored.
#
# Each test becomes a candlestick: thin wick = min..max, box = mean ± stddev,
# horizontal line = median. Y is time in ms (log scale, since tests span ~2ms
# to ~1300ms). Lower is faster.
#
# Usage:
#     plot_lantest.pl INPUT.csv [-o OUTPUT_BASENAME] [--title TITLE]
#                     [--subtitle SUBTITLE] [--meta KEY=VALUE ...]
#
# Writes OUTPUT_BASENAME.png (default basename: "lantest").

use strict;
use warnings;
use Getopt::Long qw(GetOptions);

# Near-white canvas (RGB 246,248,250) for a clean modern background.
my $BACKGROUND_COLOR = '#F6F8FA';
my $BOX_COLOR        = '#1f77b4';
my $MEDIAN_COLOR     = '#d62728';
# Max caps alpha-blended (#80 = 50% transparent) so they read lighter.
my $MAX_COLOR = '#80d62728';
my $COV_COLOR = '#ff7f0e';

# 11 x 5.2 inches at 130 dpi, matching the former matplotlib renderer.
my $WIDTH_PX  = 1430;
my $HEIGHT_PX = 590;

# Match the 6 trailing numeric fields so a comma-containing test name (group
# 1) is left intact: Time_ms,Time±,Min_ms,Max_ms,Median_ms,MB/s.
my $ROW_TAIL_RE = qr/^(.+),(\d+),([\d.]+),(\d+),(\d+),(\d+),(\d+)\s*$/;

# Map each lantest test (matched by a substring of its full name) to a short
# axis label. Order matters: first matching substring wins.
my @SHORT_LABELS = (
                    ['Writing one large file' => 'Write 100MB'],
                    ['Reading one large file' => 'Read 100MB'],
                    ['Creating 2000 files'    => 'Create 2k'],
                    ['Writing 1024 bytes'     => 'Write 2k'],
                    ['Lock then unlock 2000'  => 'Fork lock 2k'],
                    ['Stat, open, read'       => 'Stat/open/read 2k'],
                    ['Enumerate dir'          => 'Enumerate 2k'],
                    ['Deleting 2000 files'    => 'Delete 2k'],
                    ['Byte-range lock/unlock' => 'Byte lock 2k'],
                    ['Create directory tree'  => 'Create tree'],
                    ['Directory cache hits'   => 'Dircache hits'],
                    ['Mixed cache operations' => 'Mixed cache'],
                    ['Deep path traversal'    => 'Deep traverse'],
                    ['Cache validation'       => 'Cache validate'],
);

# Map a verbose test name to a short axis label, falling back to the name with
# its trailing "[N AFP ops]" annotation stripped if no keyword matches.
sub short_label {
    my ($name) = @_;
    for my $pair (@SHORT_LABELS) {
        my ($needle, $label) = @$pair;
        return $label if index($name, $needle) >= 0;
    }
    my $stripped = $name;
    $stripped =~ s/^\s+|\s+$//g;
    if ($stripped =~ /\]$/) {
        my $bracket = rindex($stripped, '[');
        $stripped = substr($stripped, 0, $bracket) if $bracket >= 0;
    }
    $stripped =~ s/^\s+|\s+$//g;
    return $stripped;
}

# Parse afp_lantest CSV into per-test hashes in file (execution) order.
sub parse_csv {
    my ($path) = @_;
    my @tests;
    open my $fh, '<', $path or die "cannot open $path: $!\n";
    while (my $line = <$fh>) {
        $line =~ s/^\s+|\s+$//g;
        next if $line eq '' || index($line, 'Sum of all AFP OPs') == 0;
        next unless $line =~ $ROW_TAIL_RE;
        push @tests, {
                      name   => $1,
                      label  => short_label($1),
                      mean   => $2 + 0,
                      stddev => $3 + 0,
                      min    => $4 + 0,
                      max    => $5 + 0,
                      median => $6 + 0,
        };
    }
    close $fh;
    return \@tests;
}

# Run-config lines shown under the chart subtitle.
sub build_info_lines {
    my ($tests, $meta) = @_;
    my @lines = ('Tests: ' . scalar(@$tests));
    if ($meta->{iterations}) {
        my $warm = $meta->{warmup} ? " +$meta->{warmup} warmup" : '';
        push @lines, "Iterations: $meta->{iterations}$warm";
    }
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

# Escape a string for a double-quoted gnuplot string (newline -> \n escape).
sub gp_quote {
    my ($s) = @_;
    $s =~ s/\\/\\\\/g;
    $s =~ s/"/\\"/g;
    $s =~ s/\n/\\n/g;
    return $s;
}

sub plot {
    my ($tests, $out_base, $title, $subtitle, $meta) = @_;
    if (!@$tests) {
        print STDERR "ERROR: no lantest rows found in input — was " . "afp_lantest run with -c (CSV)?\n";
        return 0;
    }

    my $n = scalar @$tests;
    my ($data_min, $data_max) = ($tests->[0]{min}, $tests->[0]{max});
    for my $t (@$tests) {
        $data_min = $t->{min} if $t->{min} < $data_min;
        $data_max = $t->{max} if $t->{max} > $data_max;
    }
    my $y_lo = $data_min / 1.6;
    $y_lo = 0.1 if $y_lo < 0.1;
    my $y_hi = $data_max * 1.6;

    # Columns per candlestick: x, box_lo, wick_lo, box_hi, wick_hi, median,
    # cov. Box = mean ± 1 stddev clamped to the wick; cov (stddev/mean %) is
    # the normalized jitter plotted as bars in the lower panel.
    my $rows    = '';
    my $cov_max = 0;
    my (@tics, @blank_tics);
    my $x = 0;
    for my $t (@$tests) {
        my $box_lo = $t->{mean} - $t->{stddev};
        my $box_hi = $t->{mean} + $t->{stddev};
        $box_lo = $t->{min} if $box_lo < $t->{min};
        $box_hi = $t->{max} if $box_hi > $t->{max};
        my $cov = $t->{mean} > 0 ? 100 * $t->{stddev} / $t->{mean} : 0;
        $cov_max = $cov if $cov > $cov_max;
        $rows .= "$x $box_lo $t->{min} $box_hi $t->{max} $t->{median} $cov\n";
        push @tics, sprintf('"%s" %d', gp_quote($t->{label}), $x);
        push @blank_tics, sprintf('"" %d', $x);
        $x++;
    }
    my $tics       = join(', ', @tics);
    my $blank_tics = join(', ', @blank_tics);
    # Headroom above the tallest jitter bar (floor for an all-quiet run); a
    # coarse tick interval keeps the thin band uncluttered.
    my $cov_hi  = $cov_max > 0 ? $cov_max * 1.25 : 5;
    my $cov_tic = $cov_hi > 20 ? 20              : ($cov_hi > 10 ? 10 : 5);

    my @header;
    push @header, $subtitle if defined $subtitle && $subtitle ne '';
    my @info = build_info_lines($tests, $meta);
    push @header, join(' · ', @info) if @info;
    my $header = gp_quote(join("\n", @header));

    my $suptitle = gp_quote($title || 'Netatalk AFP Lantest — per-test latency');
    my $png      = "$out_base.png";
    my $xmax     = $n - 0.4;

    my $gp_bin = $ENV{GNUPLOT_BIN} || 'gnuplot';
    open my $gp, '|-', $gp_bin or die "cannot run $gp_bin: $!\n";
    print $gp <<"EOF";
# fontscale/linewidth/pointscale 1.8 matches matplotlib's dpi=130 (130/72),
# so the point sizes below mirror the old matplotlib values. DejaVu Sans is
# matplotlib's default; pangocairo falls back to system sans if it is absent.
set terminal pngcairo noenhanced size $WIDTH_PX,$HEIGHT_PX \\
    fontscale 1.8 linewidth 1.8 pointscale 1.8 \\
    background rgb "$BACKGROUND_COLOR" font "DejaVu Sans,8"
set output "@{[ gp_quote($png) ]}"

set border 15 linewidth 0.8 linecolor rgb "#333333"
set tics nomirror out scale 0.6

# Title block manually placed above the panels for a fixed title/subtitle gap.
set label 1 "$suptitle" at screen 0.5, screen 0.95 center \\
    font "DejaVu Sans:Bold,12"
set label 2 "$header" at screen 0.5, screen 0.885 center \\
    font "DejaVu Sans,5.5" textcolor rgb "#222222"

# Pin lmargin/rmargin so the two stacked panels share an x extent (otherwise
# gnuplot sizes each to its own y-tick width and the columns misalign).
set xrange [-0.6:$xmax]
set lmargin at screen 0.075
set rmargin at screen 0.98
set boxwidth 0.5
# Y-axis titles as screen-pinned rotated labels (not per-panel "set ylabel"),
# centered on each panel's height so they align despite differing tick widths.
# Upper panel y 0.34-0.84 (mid 0.59), lower 0.18-0.30 (mid 0.24).
set label 5 "Time (ms) — lower is faster" at screen 0.022, screen 0.59 \\
    center rotate by 90 font "DejaVu Sans,7"
set label 6 "Jitter (CoV %)" at screen 0.022, screen 0.24 \\
    center rotate by 90 font "DejaVu Sans,7"
set label 7 \\
    "Coefficient of Variation (CoV) = stddev ÷ mean, as a normalized percent" \\
    at screen 0.5, screen 0.06 center font "DejaVu Sans,6.5" \\
    textcolor rgb "#444444"
# Dotted grid; minor (per-decade log) lines fainter so they don't read heavy.
set grid ytics mytics linetype 1 dashtype (1,2) linewidth 1 \\
    linecolor rgb "#BBBBBB", linetype 1 dashtype (1,2) linewidth 1 \\
    linecolor rgb "#DDDDDD"

\$DATA << EOD
$rows
EOD

set multiplot

# --- Upper panel: latency candlesticks (~70%; log y needs less height) ---
set tmargin at screen 0.84
set bmargin at screen 0.34
set logscale y
set yrange [$y_lo:$y_hi]
set format y "%g"
set ytics font ",6"
# Blank x labels here; the lower panel labels the shared ticks.
set xtics ($blank_tics)
# Vertical dividers bracketing the shared-file pipeline (Create..Delete 2k),
# drawn in both panels so the related series reads as one group.
set arrow 1 from 1.5, graph 0 to 1.5, graph 1 nohead \\
    dashtype (4,3) linewidth 1 linecolor rgb "#999999" front
set arrow 2 from 7.5, graph 0 to 7.5, graph 1 nohead \\
    dashtype (4,3) linewidth 1 linecolor rgb "#999999" front
# Legend in the free top-left outer canvas, outside the plot area so it never
# overlaps a candle.
set key at screen 0.005, screen 0.995 top left Left reverse samplen 1.5 \\
    width -1 spacing 1.1 font ",6" \\
    box linewidth 0.6 linecolor rgb "#CCCCCC" opaque fillcolor rgb "#F5F5F5"

# Whisker caps at 0.48 boxwidth = matplotlib's ±0.12 caps. The dotted cap is
# the max (slowest run). keyentry items give one legend row per candle element.
plot \$DATA using 1:2:3:5:4 with candlesticks whiskerbars 0.48 \\
        linecolor rgb "$BOX_COLOR" linewidth 1 \\
        fillstyle transparent solid 0.35 border lc rgb "$BOX_COLOR" \\
        notitle, \\
     \$DATA using (\$1 - 0.25):6:(0.5):(0.0) with vectors nohead \\
        linewidth 2 linecolor rgb "$MEDIAN_COLOR" notitle, \\
     \$DATA using (\$1 - 0.24):5:(0.48):(0.0) with vectors nohead \\
        linewidth 1 dashtype (2,1) linecolor rgb "$MAX_COLOR" notitle, \\
     \$DATA using 1:5:(sprintf("%d", \$6)) with labels \\
        offset 0,0.6 font ",5" textcolor rgb "$MEDIAN_COLOR" notitle, \\
     keyentry with lines linecolor rgb "$BOX_COLOR" linewidth 1 \\
        title "min–max range", \\
     keyentry with boxes fillstyle transparent solid 0.35 \\
        border lc rgb "$BOX_COLOR" title "mean ± std dev", \\
     keyentry with lines linecolor rgb "$MEDIAN_COLOR" linewidth 2 \\
        title "median", \\
     keyentry with lines linewidth 1 dashtype (2,1) \\
        linecolor rgb "$MAX_COLOR" title "max (slowest run)"

# --- Lower panel: jitter (CoV) bars (~20%, a thin band) ---
unset label 1
unset label 2
unset logscale y
unset key
set tmargin at screen 0.30
set bmargin at screen 0.18
set yrange [0:$cov_hi]
set format y "%g"
set ytics $cov_tic font ",6"
set xtics rotate by 12 right font ",6" ($tics)
# Narrower than the 0.5 candle boxes so the jitter bars read as a distinct mark.
set boxwidth 0.3
plot \$DATA using 1:7 with boxes \\
        fillstyle solid 0.7 border lc rgb "$COV_COLOR" \\
        linecolor rgb "$COV_COLOR" notitle

unset multiplot
EOF
    close $gp or die "gnuplot failed (status $?)\n";
    print STDERR "Wrote $png\n";
    return 1;
}

sub main {
    my $output   = 'lantest';
    my $title    = undef;
    my $subtitle = undef;
    my @meta_args;
    GetOptions(
               'o|output=s' => \$output,
               'title=s'    => \$title,
               'subtitle=s' => \$subtitle,
               'meta=s'     => \@meta_args,
      )
      or die "usage: plot_lantest.pl INPUT.csv [-o BASENAME] " . "[--title T] [--subtitle S] [--meta KEY=VALUE ...]\n";
    my $input = shift @ARGV
      or die "missing input CSV (afp_lantest -c capture)\n";

    my %meta;
    for my $pair (@meta_args) {
        next unless index($pair, '=') >= 0;
        my ($k, $v) = split /=/, $pair, 2;
        s/^\s+|\s+$//g for ($k, $v);
        $meta{$k} = $v;
    }

    my $tests = parse_csv($input);
    return plot($tests, $output, $title, $subtitle, \%meta) ? 0 : 1;
}

exit main();
