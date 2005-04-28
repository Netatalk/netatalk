#!/bin/sh
#
# This filter is called by psf to convert "other" formats to PostScript.
# psf handles text and PostScript native.  "Other" formats, e.g. DVI, C/A/T,
# need to be converted before the page reverser and the printer can use
# them.
#
# $0 begins with the filter name, e.g. df, tf.  Each format is a separate
# tag in the case.
#

DVIPSPATH=/usr/local/tex/bin
DVIPS=/usr/local/tex/bin/dvips
DVIPSARGS="-f -q"

TROFF2PS=/usr/local/psroff/troff2/troff2ps
TROFF2PSARGS="-Z -O-.10"

PATH=/usr/bin:$DVIPSPATH; export PATH

case $1 in

#
# Use "dvips" by Radical Eye Software to convert TeX DVI files to PostScript.
# Note that you *must* have METAFONT, etc, in your path.
#
df*)
    if [ -x "$DVIPS" ]; then
	TEMPFILE=`mktemp -t psfilter.XXXXXX` || exit 1
	cat > $TEMPFILE
	$DVIPS $DVIPSARGS < $TEMPFILE
	rm -f $TEMPFILE
    else
	echo "$0: filter dvips uninstalled" 1>&2
	exit 2
    fi
    ;;

#
# troff2ps is from psroff by Chris Lewis.
#
tf*)
    if [ -x "$TROFF2PS" ]; then
	exec $TROFF2PS $TROFF2PSARGS
    else
	echo "$0: filter troff2ps uninstalled" 1>&2
	exit 2
    fi
    ;;

*)
    echo "$0: filter $1 unavailable" 1>&2
    exit 2
    ;;
esac

exit 0
