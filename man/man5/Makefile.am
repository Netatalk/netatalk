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

GENERATED_MANS = AppleVolumes.default.5 \
	afpd.conf.5 \
	netatalk.conf.5 \
	afp_ldap.conf.5 \
	afp_signature.conf.5 \
	afp_voluuid.conf.5

TEMPLATE_FILES = AppleVolumes.default.5.tmpl \
	afpd.conf.5.tmpl \
	netatalk.conf.5.tmpl \
	afp_ldap.conf.5.tmpl \
	afp_signature.conf.5.tmpl \
	afp_voluuid.conf.5.tmpl

ATALK_MANS = atalkd.conf.5.tmpl papd.conf.5.tmpl

if USE_APPLETALK
GENERATED_MANS += atalkd.conf.5 papd.conf.5
TEMPLATE_FILES += $(ATALK_MANS)
endif

NONGENERATED_MANS = AppleVolumes.5 AppleVolumes.system.5

man_MANS = $(GENERATED_MANS) $(NONGENERATED_MANS)

CLEANFILES = $(GENERATED_MANS)

EXTRA_DIST = $(TEMPLATE_FILES) $(NONGENERATED_MANS) $(ATALK_MANS)
