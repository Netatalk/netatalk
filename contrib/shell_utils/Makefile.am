# Makefile for contrib/shell_utils/

pkgconfdir = @PKGCONFDIR@

GENERATED_FILES = \
	afpd-mtab.pl			\
	apple_cp apple_mv apple_rm	\
	cleanappledouble.pl		\
	lp2pap.sh			\
	netatalkshorternamelinks.pl
TEMPLATE_FILES = $(foreach f,$(GENERATED_FILES),$(f).in)

CLEANFILES = $(GENERATED_FILES)

bin_SCRIPTS = $(GENERATED_FILES)

EXTRA_DIST = $(TEMPLATE_FILES)
