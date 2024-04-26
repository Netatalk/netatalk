# Makefile for contrib/shell_utils/

pkgconfdir = @PKGCONFDIR@

GENERATED_FILES =
TEMPLATE_FILES =
PERLSCRIPTS = \
	afpstats \
	apple_dump \
	asip-status

SUFFIXES = .tmpl .

.tmpl:
	sed -e s@:BINDIR:@${bindir}@ \
		-e s@:SBINDIR:@${sbindir}@ \
		-e s@:ETCDIR:@${pkgconfdir}@ \
		<$< >$@

CLEANFILES = $(GENERATED_FILES)

bin_SCRIPTS = $(PERLSCRIPTS) $(GENERATED_FILES)

EXTRA_DIST = $(TEMPLATE_FILES) make-casetable.pl make-precompose.h.pl fce_ev_script.sh docker-entrypoint.sh
