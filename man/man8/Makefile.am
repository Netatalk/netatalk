## Makefile.am for man/man8/

pkgconfdir = @PKGCONFDIR@

SUFFIXES = .tmpl .

.tmpl:
	sed -e s@:SBINDIR:@${sbindir}@ \
	    -e s@:BINDIR:@${bindir}@ \
	    -e s@:ETCDIR:@${pkgconfdir}@ \
	    -e s@:LIBDIR:@${libdir}@ \
	    <$^ >$@

NONGENERATED_MANS = timelord.8
GENERATED_MANS    = afpd.8 atalkd.8 pap.8 papd.8 papstatus.8 psf.8
TEMPLATE_FILES    = $(foreach f,$(GENERATED_MANS),$(f).tmpl)

man_MANS = $(GENERATED_MANS) $(NONGENERATED_MANS)

CLEANFILES += $(GENERATED_MANS)

EXTRA_DIST = $(TEMPLATE_FILES) $(NONGENERATED_MANS)
