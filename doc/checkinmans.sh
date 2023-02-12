#!/bin/sh

#
# Copies man pages from dir "manpages" to netalk CVS working copy man directory structure <dir>
# As some man pages contain path variables that are substituded at build time we have to be carefull:
# we try to figure out which they are by examing the Makefile.am's and append ".tml" to any of them.
#

if test $# -ne 1; then
    echo "Usage: checkinmans <netatalk source dir>"
    exit 1
fi

CURRDIR="`pwd`"
WORKDIR="$CURRDIR/manpages"
MANDIR="$1/man"

cd "$MANDIR"

for am in */*.am; do
    echo Examining Makefile.am \"$am\"
    FILES="$(grep -Ev "SUFFIXES|.tmpl:" $am | grep tmpl | sed -e "s/TEMPLATE_FILES.*=//g" -e "s/ATALK_MANS.*=//g" -e "s/.tmpl//g" | tr -d '\n' | tr -d '\\')"
    echo Found generated files: \"$FILES\"
    GENERATED="$GENERATED $FILES"
done

echo "Generated manpages are: $GENERATED"

cd "$WORKDIR"

for section in 1 3 4 5 8 ; do
    echo "Now processing man pages from section $section "
    files=`find . -name "*.$section" -print`
    for file in $files ; do
	file=`basename $file`
	echo "    man page: $file"
	g=0
	for gen in $GENERATED ; do
	    if test "x$file" = "x$gen" ; then
		echo '              generated man, appendig ".tmpl"'
		cp "$file" "$MANDIR/man$section/$file.tmpl"
		g=1
	    fi
	done
	if test $g -eq 0 ; then
	    cp "$file" "$MANDIR/man$section/$file"
	fi
    done
done

exit 0