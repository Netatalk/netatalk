#!/usr/bin/perl 
# 
# (c) 2000 Christian Wolff, scarabaeus@scarabaeus.org
# quick hack to create symbolic links for files with names over 31 chars long 
# 

$searchpath='/data/mp3/';
$destpath='/data/mac_mp3/';

# only if you dare!
`rm -rf ${destpath}*`;
foreach $f (`find $searchpath -name '*.mp3'`) {
	chomp $f;
	$f=~s/^$searchpath//;
	if ($f=~/^(.*)\/(.*)$/) {
		($path,$file)=($1,$2);
	} else {
		($path,$file)=('',$f);
		}
	$shortpath='';
	for $splitpath (split /\//,$path) {
		if (length $splitpath > 31) {
			# keep the last 2 chars of the directory name
			$splitpath=substr($splitpath,0,29).substr($splitpath,-2,2);
			}
		$shortpath.="${splitpath}/";
		mkdir $destpath.$shortpath,0755;
	}
	$shortfile=$file;
	if (length $file > 31) {
		# keep the extension of 4 chars
		$shortfile=substr($file,0,27).substr($file,-4,4);
	}
	`ln -sf ${searchpath}${f} ${destpath}${shortpath}${shortfile}`;
}

