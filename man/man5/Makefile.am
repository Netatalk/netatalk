# Makefile.am for man/man5/

pkgconfdir = @PKGCONFDIR@

SUFFIXES = .tmpl .

.tmpl:
	sed -e "s@:SBINDIR:@${sbindir}@g" \
	    -e "s@:BINDIR:@${bindir}@g" \
	    -e "s@:ETCDIR:@${pkgconfdir}@g" \
	    -e "s@:LIBDIR:@${libdir}@g" \
		-e "s@:DEFAULT_CNID_SCHEME:@${DEFAULT_CNID_SCHEME}@g" \
		-e "s@:COMPILED_BACKENDS:@${compiled_backends}@g" \
	    <$< >$@

man_MANS = AppleVolumes.default.5 afpd.conf.5 \
	atalkd.conf.5 netatalk.conf.5 papd.conf.5 \
	afp_ldap.conf.5

TEMPLATE_FILES = AppleVolumes.default.5.tmpl afpd.conf.5.tmpl \
	atalkd.conf.5.tmpl netatalk.conf.5.tmpl papd.conf.5.tmpl \
	afp_ldap.conf.5.tmpl

CLEANFILES = $(man_MANS)

EXTRA_DIST = $(TEMPLATE_FILES) 
