#!/bin/sh

libtoolize --force && aclocal $ACLOCAL_FLAGS && autoheader && automake --add-missing && autoconf

# just in case automake generated errors...
autoconf
