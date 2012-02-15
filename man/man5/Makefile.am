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

GENERATED_MANS = afp_signature.conf.5 afp_voluuid.conf.5
TEMPLATE_FILES = afp_signature.conf.5.tmpl afp_voluuid.conf.5.tmpl
NONGENERATED_MANS =

man_MANS = $(GENERATED_MANS) $(NONGENERATED_MANS)

CLEANFILES = $(GENERATED_MANS)

EXTRA_DIST = $(TEMPLATE_FILES) $(NONGENERATED_MANS)
