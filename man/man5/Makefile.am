# Makefile.am for man/man5/

SUFFIXES = .in .

.in:
	sed -e "s,[@]pkgconfdir[@],${sysconfdir},g" \
		-e "s,[@]localstatedir[@],${localstatedir}," \
		-e "s,[@]prefix[@],${prefix}," \
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
DISTCLEANFILES = $(man_MANS)
noinst_DATA = $(man_MANS)
