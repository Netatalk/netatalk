#!/usr/bin/env perl
#
# usage: make-precompose.h.pl <infile> <outfile>
#        make-precompose.h.pl UnicodeData.txt precompose.h
#
# (c) 2008-2011 by HAT <hat@fa2.so-net.ne.jp>
# (c) 2024 by Daniel Markstedt <daniel@mindani.net>
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

# See
# https://www.unicode.org/reports/tr44/
# https://www.unicode.org/reports/tr15/
# https://www.unicode.org/Public/UNIDATA/UnicodeData.txt

# temp files for binary search (compose.TEMP, compose_sp.TEMP) -------------

open(UNICODEDATA, "<$ARGV[0]") or die "$0: open $ARGV[0]: $!";
open(CHEADER,     ">$ARGV[1]") or die "$0: open $ARGV[1]: $!";

open(COMPOSE_TEMP,    ">compose.TEMP")    or die "$0: open compose.TEMP: $!";
open(COMPOSE_SP_TEMP, ">compose_sp.TEMP") or die "$0: open compose_sp.TEMP: $!";

while (<UNICODEDATA>) {
    chop;
    (
     $code0,
     $Name1,
     $General_Category2,
     $Canonical_Combining_Class3,
     $Bidi_Class4,
     $Decomposition_Mapping5,
     $Numeric_Value6,
     $Numeric_Value7,
     $Numeric_Value8,
     $Bidi_Mirrored9,
     $Unicode_1_Name10,
     $ISO_Comment11,
     $Simple_Uppercase_Mapping12,
     $Simple_Lowercase_Mapping13,
     $Simple_Titlecase_Mapping14
    ) = split(/\;/);

    if (($Decomposition_Mapping5 ne "") && ($Decomposition_Mapping5 !~ /\</) && ($Decomposition_Mapping5 =~ / /)) {
        ($base, $comb) = split(/ /, $Decomposition_Mapping5);

        $leftbracket  = "    { ";
        $rightbracket = " },     ";

        # AFP 3.x Spec
        if (    ((0x2000 <= hex($code0)) && (hex($code0) <= 0x2FFF))
             || ((0xFE30 <= hex($code0))  && (hex($code0) <= 0xFE4F))
             || ((0x2F800 <= hex($code0)) && (hex($code0) <= 0x2FA1F))) {
            $leftbracket  = "    \/\*{ ";
            $rightbracket = " },\*\/   ";
        }

        if (hex($code0) > 0xFFFF) {

            $code0_sp_hi = 0xD800 - (0x10000 >> 10) + (hex($code0) >> 10);
            $code0_sp_lo = 0xDC00 + (hex($code0) & 0x3FF);

            $base_sp_hi = 0xD800 - (0x10000 >> 10) + (hex($base) >> 10);
            $base_sp_lo = 0xDC00 + (hex($base) & 0x3FF);

            $comb_sp_hi = 0xD800 - (0x10000 >> 10) + (hex($comb) >> 10);
            $comb_sp_lo = 0xDC00 + (hex($comb) & 0x3FF);

            printf(COMPOSE_SP_TEMP "%s0x%04X%04X, 0x%04X%04X, 0x%04X%04X%s\/\* %s \*\/\n",
                   $leftbracket,  $code0_sp_hi, $code0_sp_lo, $base_sp_hi, $base_sp_lo, $comb_sp_hi, $comb_sp_lo,
                   $rightbracket, $Name1
            );

            $leftbracket  = "    \/\*{ ";
            $rightbracket = " },\*\/   ";
        }

        printf(COMPOSE_TEMP "%s0x%08X, 0x%08X, 0x%08X%s\/\* %s \*\/\n", $leftbracket, hex($code0), hex($base),
               hex($comb), $rightbracket, $Name1
        );

    }
}

close(UNICODEDATA);

close(COMPOSE_TEMP);
close(COMPOSE_SP_TEMP);

# macros for BMP (PRECOMP_COUNT, DECOMP_COUNT, MAXCOMBLEN) ----------------

open(COMPOSE_TEMP, "<compose.TEMP") or die "$0: open compose.TEMP: $!";

@comp_table = ();
$comp_count = 0;

while (<COMPOSE_TEMP>) {
    if (m/^\/\*/) {
        next;
    }
    $comp_table[$comp_count][0] = substr($_, 4,  10);
    $comp_table[$comp_count][1] = substr($_, 16, 10);
    $comp_count++;
}

# Hangul's maxcomblen is already 2. That is, VT.
$maxcomblen = 2;

for ($i = 0 ; $i < $comp_count ; $i++) {
    $base    = $comp_table[$i][1];
    $comblen = 1;
    $j       = 0;
    while ($j < $comp_count) {
        if ($base ne $comp_table[$j][0]) {
            $j++;
            next;
        } else {
            $comblen++;
            $base = $comp_table[$j][1];
            $j    = 0;
        }
    }
    $maxcomblen = ($maxcomblen > $comblen) ? $maxcomblen : $comblen;
}

close(COMPOSE_TEMP);

# macros for SP (PRECOMP_SP_COUNT,DECOMP_SP_COUNT, MAXCOMBSPLEN) -----------

open(COMPOSE_SP_TEMP, "<compose_sp.TEMP") or die "$0: open compose_sp.TEMP: $!";

@comp_sp_table = ();
$comp_sp_count = 0;

while (<COMPOSE_SP_TEMP>) {
    if (m/^\/\*/) {
        next;
    }
    $comp_sp_table[$comp_sp_count][0] = substr($_, 4,  10);
    $comp_sp_table[$comp_sp_count][1] = substr($_, 16, 10);
    $comp_sp_count++;
}

# one char have 2 codepoints, like a D8xx DCxx.
$maxcombsplen = 2;

for ($i = 0 ; $i < $comp_sp_count ; $i++) {
    $base_sp = $comp_sp_table[$i][1];
    $comblen = 2;
    $j       = 0;
    while ($j < $comp_sp_count) {
        if ($base_sp ne $comp_sp_table[$j][0]) {
            $j++;
            next;
        } else {
            $comblen += 2;
            $base_sp = $comp_sp_table[$j][1];
            $j       = 0;
        }
    }
    $maxcombsplen = ($maxcombsplen > $comblen) ? $maxcombsplen : $comblen;
}

close(COMPOSE_SP_TEMP);

# macro for buffer length (COMBBUFLEN) -------------------------------------

$combbuflen = ($maxcomblen > $maxcombsplen) ? $maxcomblen : $maxcombsplen;

# sort ---------------------------------------------------------------------

system("sort -k 3 compose.TEMP \> precompose.SORT");
system("sort -k 2 compose.TEMP \>  decompose.SORT");

system("sort -k 3 compose_sp.TEMP \> precompose_sp.SORT");
system("sort -k 2 compose_sp.TEMP \>  decompose_sp.SORT");

# print  -------------------------------------------------------------------

print(CHEADER "\/\*\n");
print(CHEADER "  DO NOT EDIT BY HAND\!\!\!\n");
print(CHEADER "  This file is generated by:\n");
print(CHEADER "    make-precompose.h.pl\n");
print(CHEADER "\n");
print(CSOURCE "  The Unicode Character Database is sourced from:\n");
print(CHEADER "    https\:\/\/www.unicode.org\/Public\/UNIDATA\/UnicodeData.txt\n");
print(CHEADER "\*\/\n");
print(CHEADER "\n");

print(CHEADER "\#define SBASE 0xAC00\n");
print(CHEADER "\#define LBASE 0x1100\n");
print(CHEADER "\#define VBASE 0x1161\n");
print(CHEADER "\#define TBASE 0x11A7\n");
print(CHEADER "\#define LCOUNT 19\n");
print(CHEADER "\#define VCOUNT 21\n");
print(CHEADER "\#define TCOUNT 28\n");
print(CHEADER "\#define NCOUNT 588     \/\* (VCOUNT \* TCOUNT) \*\/\n");
print(CHEADER "\#define SCOUNT 11172   \/\* (LCOUNT \* NCOUNT) \*\/\n");
print(CHEADER "\n");

printf(CHEADER "\#define PRECOMP_COUNT %d\n", $comp_count);
printf(CHEADER "\#define DECOMP_COUNT %d\n",  $comp_count);
printf(CHEADER "\#define MAXCOMBLEN %d\n",    $maxcomblen);
print(CHEADER "\n");
printf(CHEADER "\#define PRECOMP_SP_COUNT %d\n", $comp_sp_count);
printf(CHEADER "\#define DECOMP_SP_COUNT %d\n",  $comp_sp_count);
printf(CHEADER "\#define MAXCOMBSPLEN %d\n",     $maxcombsplen);
print(CHEADER "\n");
printf(CHEADER "\#define COMBBUFLEN %d  \/\* max\(MAXCOMBLEN\,MAXCOMBSPLEN\) \*\/\n", $combbuflen);
print(CHEADER "\n");

print(CHEADER "static const struct \{\n");
print(CHEADER "    unsigned int replacement\;\n");
print(CHEADER "    unsigned int base\;\n");
print(CHEADER "    unsigned int comb\;\n");
print(CHEADER "\} precompositions\[\] \= \{\n");

my $precompose_file = "precompose.SORT";
open(my $fh, "<", $precompose_file) or die "Could not open file '$precompose_file' $!";
while (my $line = <$fh>) {
    print(CHEADER $line);
}
close($fh);

print(CHEADER "\}\;\n");
print(CHEADER "\n");

print(CHEADER "static const struct \{\n");
print(CHEADER "    unsigned int replacement\;\n");
print(CHEADER "    unsigned int base\;\n");
print(CHEADER "    unsigned int comb\;\n");
print(CHEADER "\} decompositions\[\] \= \{\n");

my $decompose_file = "decompose.SORT";
open(my $fh, "<", $decompose_file) or die "Could not open file '$decompose_file' $!";
while (my $line = <$fh>) {
    print(CHEADER $line);
}
close($fh);

print(CHEADER "\}\;\n");
print(CHEADER "\n");

print(CHEADER "static const struct \{\n");
print(CHEADER "    unsigned int replacement_sp\;\n");
print(CHEADER "    unsigned int base_sp\;\n");
print(CHEADER "    unsigned int comb_sp\;\n");
print(CHEADER "\} precompositions_sp\[\] \= \{\n");

my $precompose_sp_file = "precompose_sp.SORT";
open(my $fh, "<", $precompose_sp_file) or die "Could not open file '$precompose_sp_file' $!";
while (my $line = <$fh>) {
    print(CHEADER $line);
}
close($fh);

print(CHEADER "\}\;\n");
print(CHEADER "\n");

print(CHEADER "static const struct \{\n");
print(CHEADER "    unsigned int replacement_sp\;\n");
print(CHEADER "    unsigned int base_sp\;\n");
print(CHEADER "    unsigned int comb_sp\;\n");
print(CHEADER "\} decompositions_sp\[\] \= \{\n");

my $decompose_sp_file = "decompose_sp.SORT";
open(my $fh, "<", $decompose_sp_file) or die "Could not open file '$decompose_sp_file' $!";
while (my $line = <$fh>) {
    print(CHEADER $line);
}
close($fh);

print(CHEADER "\}\;\n");
print(CHEADER "\n");

print(CHEADER "\/\* EOF \*\/\n");

close(CHEADER);

my @tempfiles = qw(compose.TEMP compose_sp.TEMP precompose.SORT decompose.SORT precompose_sp.SORT decompose_sp.SORT);

foreach my $file (@tempfiles) {
    unless (unlink $file) {
        warn "Could not remove $file: $!\n";
    }
}

# EOF
