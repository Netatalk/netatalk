#! /usr/bin/perl
#
# cleanappledouble.pl
# Copyright (C) 2001 Heath Kehoe <hakehoe@avalon.net>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#

require 5;
use Getopt::Std;

sub usage {
    print STDERR <<EOF;
Usage: $0 [-r] [-v] directory [directory ...]

Scans each directory and:
 1) removes orphaned .AppleDouble files (from <directory>/.AppleDouble)
 2) fixes permissions on .AppleDouble files to match corresponding data file (minus x bits)
 3) fixes owner/group of .AppleDouble files to match corresponding data file (root only)

Options:
  -r   Recursively check all subdirectories of each directory
  -R   Like -r but follows symbolic links to directories (warning: no loop-checking is done)
  -p   Preview: no deletions or changes are actually made
  -v   Verbose
EOF
    exit 1;
}

$isroot = ($> == 0);

sub S_ISDIR {
    my($mode) = @_;
    return (($mode & 0170000) == 0040000);
}
sub S_ISREG {
    my($mode) = @_;
    return (($mode & 0170000) == 0100000);
}
sub S_ISLNK {
    my($mode) = @_;
    return (($mode & 0170000) == 0120000);
}


sub do_dir {
    my($dir) = @_;
    my($f, $havead, @dirlist, $mode, $uid, $gid, $fn);
    my(%dm, %du, %dg);

    print STDERR "Scanning $dir ...\n" if($opt_v);

    $havead = -d "$dir/.AppleDouble";

    # there's nothing more to do if we're not recursive and there's no .AppleDouble
    return if(!$havead && !$opt_r);

    opendir DIR, $dir	or do {
		warn "Can't opendir $dir: $!\n";
		return;
    };
    while(defined($f = readdir DIR)) {
	next if($f eq ".");
	next if($f eq "..");
	next if($f eq ".AppleDouble");
	next if($f eq ".AppleDesktop");

	(undef, undef, $mode, undef, $uid, $gid) = lstat "$dir/$f";
	next if(!defined($mode));

	if(S_ISLNK($mode)) {
	    (undef, undef, $mode, undef, $uid, $gid) = stat "$dir/$f";
	    next if(!defined($mode));
	    next if(S_ISDIR($mode) && !$opt_R);
	} 
	if(S_ISDIR($mode)) {
	    push @dirlist, $f if($opt_r);
	} elsif(S_ISREG($mode)) {
	    if($havead) {
		$dm{$f} = $mode & 0666;
		$du{$f} = $uid;
		$dg{$f} = $gid;
	    }
	} else {
	    warn "Ignoring special file: $dir/$f\n";
	}
    }
    closedir DIR;

    if($havead) {
	if(opendir DIR, "$dir/.AppleDouble") {
	    while(defined($f = readdir DIR)) {
		next if($f eq ".");
		next if($f eq "..");
		next if($f eq ".Parent");

		$fn = "$dir/.AppleDouble/$f";
		(undef, undef, $mode, undef, $uid, $gid) = stat $fn;
		next if(!defined($mode));

		if(S_ISDIR($mode)) {
		    warn "Found subdirectory $f in $dir/.AppleDouble\n";
		    next;
		}
		unless(exists $dm{$f}) {
		    print STDERR "Deleting $fn ...\n" if($opt_v);
		    if($opt_p) {
			print "rm '$fn'\n";
		    } else {
			unlink "$fn" or warn "Can't unlink $fn: $!\n";
		    }
		    next;
		}
		$mode = $mode & 07777;
		if($mode != $dm{$f}) {
		    printf STDERR "Changing permissions from %o to %o on $fn\n", $mode, $dm{$f} if($opt_v);
		    if($opt_p) {
			printf "chmod %o '$fn'\n", $dm{$f};
		    } else {
			chmod $dm{$f}, $fn or warn "Can't chmod $fn: $!\n";
		    }
		}
		if($isroot && ($uid != $du{$f} || $gid != $dg{$f})) {
		    print STDERR "Changing owner from $uid:$gid to $du{$f}:$dg{$f} on $fn\n" if($opt_v);
		    if($opt_p) {
			print "chown $du{$f}:$dg{$f} '$fn'\n";
		    } else {
			chown $du{$f}, $dg{$f}, $fn or warn "Can't chown $fn: $!\n";
		    }
		}
	    }
	    closedir DIR;
	} else {
	    warn "Can't opendir $dir/.AppleDouble: $!\n";
	}
    }

    if($opt_r) {
	foreach $f ( @dirlist ) {
	    do_dir("$dir/$f");
	}
    }
}

usage unless(getopts 'prRv');
usage if(@ARGV == 0);

if($opt_R) {
    $opt_r = 1;
}

foreach $d ( @ARGV ) {
    do_dir $d;
}

