# Makefile.am for man/man5/

SUFFIXES = .in .

.in:
	sed -e s@:NETATALK_VERSION:@${NETATALK_VERSION}@ \
	    -e s@:pkgconfdir:@${sysconfdir}@ \
	    -e s@:localstatedir:@${localstatedir}@ \
	    <$< >$@

man_MANS = \
	afp.conf.5 \
	afp_signature.conf.5 \
	afp_voluuid.conf.5 \
	extmap.conf.5

TEMPLATES = \
	afp.conf.5.in \
	afp_signature.conf.5.in \
	afp_voluuid.conf.5.in \
	extmap.conf.5.in

CLEANFILES = $(man_MANS)
EXTRA_DIST = $(TEMPLATES)
noinst_DATA = $(man_MANS)
