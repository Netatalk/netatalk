## Makefile.am for man/man8/

SUFFIXES = .tmpl .

.tmpl:
	sed -e s@:SBINDIR:@${sbindir}@ \
	    -e s@:BINDIR:@${bindir}@ \
	    -e s@:ETCDIR:@${pkgconfdir}@ \
	    -e s@:LIBDIR:@${libdir}@ \
	    <$^ >$@

man_MANS = afpd.8 atalkd.8 pap.8 papd.8 papstatus.8 psf.8

CLEANFILES = afpd.8 atalkd.8 pap.8 papd.8 papstatus.8 psf.8

EXTRA_DIST = $(man_MANS)
