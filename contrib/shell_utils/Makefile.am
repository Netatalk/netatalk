# Makefile for contrib/shell_utils/

pkgconfdir = @PKGCONFDIR@

GENERATED_FILES = lp2pap.sh
TEMPLATE_FILES = lp2pap.sh.tmpl
PERLSCRIPTS = \
	afpd-mtab.pl			\
	apple_cp apple_mv apple_rm	\
	cleanappledouble.pl		\
	asip-status.pl

SUFFIXES = .tmpl .

.tmpl:
	sed -e s@:BINDIR:@${bindir}@ \
		-e s@:SBINDIR:@${sbindir}@ \
		-e s@:ETCDIR:@${pkgconfdir}@ \
		<$< >$@

CLEANFILES = $(GENERATED_FILES)

bin_SCRIPTS = $(PERLSCRIPTS) $(GENERATED_FILES)

EXTRA_DIST = $(PERLSCRIPTS) $(TEMPLATE_FILES)
