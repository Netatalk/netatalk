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

NONGENERATED_MANS = timelord.8
GENERATED_MANS    = afpd.8 cnid_dbd.8 cnid_metad.8
TEMPLATE_FILES    = afpd.8.tmpl cnid_dbd.8.tmpl cnid_metad.8.tmpl
ATALK_MANS        = atalkd.8.tmpl papd.8.tmpl papstatus.8.tmpl psf.8.tmpl

if USE_APPLETALK
GENERATED_MANS += atalkd.8 papd.8 papstatus.8 psf.8
TEMPLATE_FILES += $(ATALK_MANS)
endif

man_MANS = $(GENERATED_MANS) $(NONGENERATED_MANS)

CLEANFILES = $(GENERATED_MANS)

EXTRA_DIST = $(TEMPLATE_FILES) $(NONGENERATED_MANS) $(ATALK_MANS)
