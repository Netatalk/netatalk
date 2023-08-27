# Makefile.am for man/man1/

SUFFIXES = .in .

.in:
	sed -e "s,[@]pkgconfdir[@],${sysconfdir},g" \
		-e "s,[@]localstatedir[@],${localstatedir}," \
		-e "s,[@]prefix[@],${prefix}," \
	    <$< >$@

man_MANS = \
	ad.1 \
	afpldaptest.1 \
	afppasswd.1 \
	afpstats.1 \
	apple_dump.1 \
	asip-status.1 \
	dbd.1 \
	macusers.1 \
	netatalk-config.1 \
	uniconv.1
	
TEMPLATES = \
	ad.1.in \
	afpldaptest.1.in \
	afppasswd.1.in \
	afpstats.1.in \
	apple_dump.1.in \
	asip-status.1.in \
	dbd.1.in \
	macusers.1.in \
	netatalk-config.1.in \
	uniconv.1.in

CLEANFILES = $(man_MANS)
DISTCLEANFILES = $(man_MANS)
noinst_DATA = $(man_MANS)

