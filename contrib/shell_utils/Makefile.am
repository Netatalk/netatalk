# Makefile for contrib/shell_utils/

SUFFIXES = .tmpl .

.tmpl:
	sed -e s@:BINDIR:@${bindir}@ \
		-e s@:SBINDIR:@${sbindir}@ \
		-e s@:ETCDIR:@${CONFIG_DIR}@ \
		<$^ >$@

CLEANFILES = lp2pap.sh

bin_SCRIPTS = apple_cp apple_mv apple_rm lp2pap.sh
