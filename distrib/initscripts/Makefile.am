## Makefile for distrib/initscripts/

SUFFIXES = .tmpl .

.tmpl:
	sed -e s@:BINDIR:@${bindir}@ \
	    -e s@:SBINDIR:@${sbindir}@ \
	    -e s@:ETCDIR:@${CONFIG_DIR}@ \
	    <$^ >$@

sysvdir	= /etc/rc.d/init.d

CLEANFILES = rc.atalk.redhat atalk

#
# checking for "redhat" style sysv scripts:
#

if USE_REDHAT

sysv_SCRIPTS = atalk

atalk: rc.atalk.redhat
	cp -f rc.atalk.redhat atalk
	chmod a+x atalk

endif

#
# checking for "cobalt" style sysv scripts:
#

if USE_COBALT

sysv_SCRIPTS = atalk

atalk: rc.atalk.cobalt
	cp -f rc.atalk.cobalt atalk
	chmod a+x atalk

endif
