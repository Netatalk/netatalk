#!/bin/sh

# quick fix to remove old INSTALL/ directory
rm -R -f INSTALL

# another fix to hack move of README
touch README

# build it all.
libtoolize --force --copy && \
	aclocal $ACLOCAL_FLAGS && \
	autoheader && \
	automake --include-deps --add-missing && \
	autoconf

# just in case automake generated errors...
autoconf
