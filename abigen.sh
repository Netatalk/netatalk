#!/bin/sh
# generate a set of ABI signatures from a shared library

SHAREDLIB="$1"

GDBSCRIPT="gdb_syms.$$"

(
cat <<EOF
set height 0
set width 0
EOF
nm "$SHAREDLIB" | cut -d' ' -f2- | egrep '^[BDGTRVWS]' | grep -v @ | cut -c3- | egrep -v '^[_]' | sort | while read s; do
    echo "echo $s: "
    echo p $s
done
) > $GDBSCRIPT

# forcing the terminal avoids a problem on Fedora12
TERM=none gdb -batch -x $GDBSCRIPT "$SHAREDLIB" < /dev/null | sed -e 's/:\$[0-9]* = /: /g' -e 's/<[0-9a-zA-Z_]*>$//g' -e 's/0x[0-9a-f]* $//g' -e 's/0x[[:xdigit:]]* \("[[:alnum:] _-]*"\)/\1/g' -e 's/0x[[:xdigit:]]* \(<[[:alnum:] _-]*>\)/\1/g' -e 's/{\(.*\)} $/\1/g'
rm -f $GDBSCRIPT
