#!/bin/sh

# quick fix to remove old INSTALL/ directory
rm -Rvf INSTALL

# build it all.
libtoolize --force --copy && aclocal $ACLOCAL_FLAGS && autoheader && automake --add-missing && autoconf

# just in case automake generated errors...
autoconf
