## Makefile.am for man/man8/

pkgconfdir = @PKGCONFDIR@

SUFFIXES = .tmpl .

.tmpl:
	sed -e s@:SBINDIR:@${sbindir}@ \
	    -e s@:BINDIR:@${bindir}@ \
	    -e s@:ETCDIR:@${pkgconfdir}@ \
	    -e s@:LIBDIR:@${libdir}@ \
	    -e s@:LIBEXECDIR:@${libexecdir}@ \
	    -e s@:NETATALK_VERSION:@${NETATALK_VERSION}@ \
	    <$< >$@

GENERATED_MANS    = afpd.8 cnid_dbd.8 cnid_metad.8
TEMPLATE_FILES    = afpd.8.tmpl cnid_dbd.8.tmpl cnid_metad.8.tmpl

man_MANS = $(GENERATED_MANS)

CLEANFILES = $(GENERATED_MANS)

EXTRA_DIST = $(TEMPLATE_FILES)
