# Makefile for contrib/shell_utils/

pkgconfdir = @PKGCONFDIR@

GENERATED_FILES = lp2pap.sh
TEMPLATE_FILES = $(foreach f,$(GENERATED_FILES),$(f).tmpl)
SCRIPTS = \
	afpd-mtab.pl			\
	apple_cp apple_mv apple_rm	\
	cleanappledouble.pl		\
	netatalkshorternamelinks.pl

SUFFIXES = .tmpl .

.tmpl:
	sed -e s@:BINDIR:@${bindir}@ \
		-e s@:SBINDIR:@${sbindir}@ \
		-e s@:ETCDIR:@${pkgconfdir}@ \
		<$< >$@

CLEANFILES = $(GENERATED_FILES)

bin_SCRIPTS = $(SCRIPTS) $(GENERATED_FILES)

EXTRA_DIST = $(SCRIPTS) $(TEMPLATE_FILES)
