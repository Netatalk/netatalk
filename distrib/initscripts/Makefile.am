## Makefile for distrib/initscripts/

SUFFIXES = .tmpl .

.tmpl:
	sed -e s@:BINDIR:@${bindir}@ \
	    -e s@:SBINDIR:@${sbindir}@ \
	    -e s@:ETCDIR:@${CONFIG_DIR}@ \
	    <$^ >$@

sysvdir	= /etc/rc.d/init.d/
sysv_SCRIPTS = rc.atalk.redhat

CLEANFILES = rc.atalk.redhat

