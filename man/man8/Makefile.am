## Makefile.am for man/man8/

SUFFIXES = .in .

.in:
	sed -e "s,[@]pkgconfdir[@],${sysconfdir},g" \
		-e "s,[@]localstatedir[@],${localstatedir}," \
		-e "s,[@]prefix[@],${prefix}," \
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
DISTCLEANFILES = $(man_MANS)
noinst_DATA = $(man_MANS)
