# Makefile.am for man/man5/

pkgconfdir = @PKGCONFDIR@

SUFFIXES = .tmpl .

.tmpl:
	sed -e s@:SBINDIR:@${sbindir}@ \
	    -e s@:BINDIR:@${bindir}@ \
	    -e s@:ETCDIR:@${pkgconfdir}@ \
	    -e s@:LIBDIR:@${libdir}@ \
	    <$^ >$@

man_MANS = AppleVolumes.default.5 afpd.conf.5 \
	atalkd.conf.5 netatalk.conf.5 papd.conf.5 

TEMPLATE_FILES = $(foreach f,$(man_MANS),$(f).tmpl)

CLEANFILES += $(man_MANS)

EXTRA_DIST = $(TEMPLATE_FILES) 
