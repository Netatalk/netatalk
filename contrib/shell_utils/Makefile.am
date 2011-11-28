# Makefile for contrib/shell_utils/

pkgconfdir = @PKGCONFDIR@

GENERATED_FILES = lp2pap.sh
TEMPLATE_FILES = lp2pap.sh.tmpl
PERLSCRIPTS = \
	asip-status.pl \
	apple_dump

SUFFIXES = .tmpl .

.tmpl:
	sed -e s@:BINDIR:@${bindir}@ \
		-e s@:SBINDIR:@${sbindir}@ \
		-e s@:ETCDIR:@${pkgconfdir}@ \
		<$< >$@

CLEANFILES = $(GENERATED_FILES)

bin_SCRIPTS = $(PERLSCRIPTS) $(GENERATED_FILES)

EXTRA_DIST = $(TEMPLATE_FILES) make-casetable.pl make-precompose.h.pl
