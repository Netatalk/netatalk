## Makefile.am for man/man8/

SUFFIXES = .in .

.in:
	sed -e s@:NETATALK_VERSION:@${NETATALK_VERSION}@ \
	    -e s@:pkgconfdir:@${sysconfdir}@ \
	    -e s@:localstatedir:@${localstatedir}@ \
	    <$< >$@

man_MANS = \
	afpd.8 \
	cnid_dbd.8 \
	cnid_metad.8 \
	netatalk.8

TEMPLATES = \
	afpd.8.in \
	cnid_dbd.8.in \
	cnid_metad.8.in \
	netatalk.8.in

CLEANFILES = $(man_MANS)
EXTRA_DIST = $(TEMPLATES)
noinst_DATA = $(man_MANS)
