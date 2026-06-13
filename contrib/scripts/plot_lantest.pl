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
# Max caps in the median hue but alpha-blended (#80… = 50% transparent)
# so they read lighter than the solid median line.
my $MAX_COLOR = '#80d62728';
# Coefficient-of-variation (jitter) bars in the lower panel.
my $COV_COLOR = '#ff7f0e';

# 11 x 5.2 inches at 130 dpi, matching the former matplotlib renderer.
my $WIDTH_PX  = 1430;
my $HEIGHT_PX = 590;

# A summary data row ends with the numeric columns the candlestick needs:
#   ...,Time_ms,Time±,Min_ms,Max_ms,Median_ms,MB/s
# Match those trailing fields so a comma-containing test name is left intact.
my $ROW_TAIL_RE = qr/^(.+),(\d+),([\d.]+),(\d+),(\d+),(\d+),(\d+)\s*$/;

# Map each lantest test (matched by a substring of its full name) to a short
# axis label. Order matters: first matching substring wins.
my @SHORT_LABELS = (
                    ['Open, stat and read'    => 'Open/stat/read'],
                    ['Writing one large file' => 'Write 100MB'],
                    ['Reading one large file' => 'Read 100MB'],
                    ['Locking/Unlocking'      => 'Lock/unlock'],
                    ['Creating dir with'      => 'Create 2000'],
                    ['Enumerate dir'          => 'Enumerate'],
                    ['Deleting dir'           => 'Delete 2000'],
                    ['Create directory tree'  => 'Create tree'],
                    ['Directory cache hits'   => 'Dircache hits'],
                    ['Mixed cache operations' => 'Mixed cache'],
                    ['Deep path traversal'    => 'Deep traverse'],
                    ['Cache validation'       => 'Cache validate'],
);

# Map a verbose lantest test name to a short axis tick label. Falls back to
# the name with its trailing "[N AFP ops]" annotation stripped if no keyword
# matches (keeps new/renamed tests readable).
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

# Parse afp_lantest CSV into an ordered list of per-test hashes. Each hash
# has: name, label, mean, stddev, min, max, median. Rows are returned in
# file order (the test execution order).
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

# Compose the run-config lines shown under the chart subtitle.
sub build_info_lines {
    my ($tests, $meta) = @_;
    my @lines = ('Tests: ' . scalar(@$tests));
    if ($meta->{iterations}) {
        my $warm = $meta->{warmup} ? " +$meta->{warmup} warmup" : '';
        push @lines, "Iterations: $meta->{iterations}$warm";
    }
    push @lines, "AFP: $meta->{afp}" if $meta->{afp};
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

# Render the latency candlestick chart to <out_base>.png via gnuplot.
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
    # Same padding as the matplotlib renderer; the legend is opaque and
    # drawn on top of the data, so it needs no extra headroom.
    my $y_hi = $data_max * 1.6;

    # Candlestick columns: x, box_lo, wick_lo, box_hi, wick_hi, median, cov.
    # Box = mean ± 1 stddev, clamped to the min..max wick. cov is the
    # coefficient of variation (stddev/mean, as a percent) — a normalised
    # jitter measure plotted as bars in the lower panel.
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
    # Headroom above the tallest jitter bar; floor so an all-quiet run still
    # gives the panel a sensible scale. A coarse tick interval keeps the thin
    # band uncluttered (≈2 ticks: one near the top, plus 0).
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
# DejaVu Sans is matplotlib's default face; pangocairo falls back to the
# system sans when it is absent, so this renders everywhere. The 1.8
# fontscale/linewidth/pointscale matches matplotlib's dpi=130 rendering
# (130/72), so point sizes below mirror the old matplotlib values.
set terminal pngcairo noenhanced size $WIDTH_PX,$HEIGHT_PX \\
    fontscale 1.8 linewidth 1.8 pointscale 1.8 \\
    background rgb "$BACKGROUND_COLOR" font "DejaVu Sans,8"
set output "@{[ gp_quote($png) ]}"

# Full matplotlib-style frame: box border, ticks outward on left/bottom.
set border 15 linewidth 0.8 linecolor rgb "#333333"
set tics nomirror out scale 0.6

# Manually placed title block above the panels (a fixed gap between the bold
# title and the subtitle, like the matplotlib layout).
set label 1 "$suptitle" at screen 0.5, screen 0.95 center \\
    font "DejaVu Sans:Bold,12"
set label 2 "$header" at screen 0.5, screen 0.885 center \\
    font "DejaVu Sans,6.5" textcolor rgb "#222222"

# Shared x range and fixed side margins so the two stacked panels line up
# exactly (pinning lmargin/rmargin stops gnuplot from auto-sizing each panel
# to its own y-tick label width, which would misalign the columns).
set xrange [-0.6:$xmax]
set lmargin at screen 0.075
set rmargin at screen 0.98
set boxwidth 0.5
# Y-axis titles as rotated labels pinned at a fixed screen x, one centred on
# each panel's height. Using screen coords (not per-panel "set ylabel") keeps
# both titles vertically aligned regardless of differing tick-label widths
# (e.g. "1000" upper vs "20" lower). Upper panel spans y 0.34-0.84 (mid 0.59),
# lower spans 0.18-0.30 (mid 0.24).
set label 5 "Time (ms) — lower is faster" at screen 0.022, screen 0.59 \\
    center rotate by 90 font "DejaVu Sans,7"
set label 6 "Jitter (CoV %)" at screen 0.022, screen 0.24 \\
    center rotate by 90 font "DejaVu Sans,7"
# Caption defining CoV, in the freed space below the lower panel's x labels.
set label 7 \\
    "Coefficient of Variation (CoV) = stddev ÷ mean, as a normalised percent" \\
    at screen 0.5, screen 0.06 center font "DejaVu Sans,6.5" \\
    textcolor rgb "#444444"
# Fine dotted grid matching the speedtest chart. Major lines at #BBBBBB;
# the per-decade minor log lines a touch fainter so they don't read heavy.
set grid ytics mytics linetype 1 dashtype (1,2) linewidth 1 \\
    linecolor rgb "#BBBBBB", linetype 1 dashtype (1,2) linewidth 1 \\
    linecolor rgb "#DDDDDD"

\$DATA << EOD
$rows
EOD

set multiplot

# --- Upper panel: per-test latency candlesticks. Log y compresses the
# range, so it needs less height than a linear plot would. ---
set tmargin at screen 0.84
set bmargin at screen 0.34
set logscale y
set yrange [$y_lo:$y_hi]
# Plain millisecond integers on the log axis (no 10^n scientific ticks).
set format y "%g"
set ytics font ",6"
# x tick marks are shared, but only the lower panel labels them (blank
# labels here keep the tick positions aligned across the two panels).
set xtics ($blank_tics)
# Legend pinned to the free top-left of the outer canvas (above the panel,
# left of the centred title/subtitle) so it sits entirely outside the plot
# area and never overlaps a candle.
set key at screen 0.005, screen 0.995 top left Left reverse samplen 1.5 \\
    width -1 spacing 1.1 font ",6" \\
    box linewidth 0.6 linecolor rgb "#CCCCCC" opaque fillcolor rgb "#F5F5F5"

# Whisker caps at 0.48 of boxwidth = matplotlib's ±0.12 caps vs 0.5 box.
# A dotted cap marks the max (slowest of the runs) at the top of the range.
# The keyentry items reproduce matplotlib's legend: one entry per candle
# element rather than one per plot command.
plot \$DATA using 1:2:3:5:4 with candlesticks whiskerbars 0.48 \\
        linecolor rgb "$BOX_COLOR" linewidth 1 \\
        fillstyle transparent solid 0.35 border lc rgb "$BOX_COLOR" \\
        notitle, \\
     \$DATA using (\$1 - 0.25):6:(0.5):(0.0) with vectors nohead \\
        linewidth 2 linecolor rgb "$MEDIAN_COLOR" notitle, \\
     \$DATA using (\$1 - 0.24):5:(0.48):(0.0) with vectors nohead \\
        linewidth 1 dashtype (2,1) linecolor rgb "$MAX_COLOR" notitle, \\
     keyentry with lines linecolor rgb "$BOX_COLOR" linewidth 1 \\
        title "min–max range", \\
     keyentry with boxes fillstyle transparent solid 0.35 \\
        border lc rgb "$BOX_COLOR" title "mean ± std dev", \\
     keyentry with lines linecolor rgb "$MEDIAN_COLOR" linewidth 2 \\
        title "median", \\
     keyentry with lines linewidth 1 dashtype (2,1) \\
        linecolor rgb "$MAX_COLOR" title "max (slowest run)"

# --- Lower panel: jitter (coefficient of variation) bars. A thin band —
# only a rough magnitude is needed, so it stays narrow. ---
unset label 1
unset label 2
unset logscale y
unset key
set tmargin at screen 0.30
set bmargin at screen 0.18
set yrange [0:$cov_hi]
set format y "%g"
# Few ticks — the panel is a thin band where only rough magnitude matters.
set ytics $cov_tic font ",6"
# Only this lower panel labels the shared x ticks (the test names).
set xtics rotate by 15 right font ",6" ($tics)
# Narrower than the 0.5-wide candle boxes (so the jitter panel reads as a
# distinct mark, not a mimic of the upper panel's mean±σ boxes) but wide
# enough to register as bars.
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
