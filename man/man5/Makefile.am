# Makefile.am for man/man5/

SUFFIXES = .tmpl .

.tmpl:
	sed -e s@:SBINDIR:@${sbindir}@ \
	    -e s@:BINDIR:@${bindir}@ \
	    -e s@:ETCDIR:@${CONFIG_DIR}@ \
	    -e s@:LIBDIR:@${libdir}@ \
	    <$^ >$@

man_MANS = AppleVolumes.default.5 afpd.conf.5 netatalk.conf.5 papd.conf.5 

CLEANFILES = $(man_MANS)

EXTRA_DIST = $(man_MANS) 
