# Makefile for contrib/shell_utils/

SUFFIXES = .tmpl .

.tmpl:
	sed -e s@:BINDIR:@${bindir}@ \
		-e s@:SBINDIR:@${sbindir}@ \
		-e s@:ETCDIR:@${sysconfdir}@ \
		<$^ >$@

CLEANFILES = lp2pap.sh

bin_SCRIPTS = apple_cp apple_mv apple_rm cleanappledouble.pl lp2pap.sh \
	netatalkshorternamelinks.pl afpd-mtab.pl

EXTRA_DIST = $(bin_SCRIPTS)
