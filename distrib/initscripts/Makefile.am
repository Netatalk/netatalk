## Makefile for distrib/initscripts/

SUFFIXES = .tmpl .

pkgconfdir = @PKGCONFDIR@

#
# Template Generation
#

.tmpl:
	sed -e s@:BINDIR:@${bindir}@ \
	    -e s@:SBINDIR:@${sbindir}@ \
	    -e s@:ETCDIR:@${pkgconfdir}@ \
	    <$< >$@

GENERATED_FILES = \
	rc.atalk.redhat.tmpl		\
	rc.atalk.tru64.tmpl		\
	rc.atalk.bsd.tmpl		\
	rc.atalk.suse.tmpl
TEMPLATES = $(foreach f,$(GENERATED_FILES),$(f).tmpl)

CLEANFILES = rc.atalk.bsd rc.atalk.redhat rc.atalk.suse rc.atalk.tru64 atalk
EXTRA_DIST = $(TEMPLATES) rc.atalk.cobalt rc.atalk.sysv

install-data-hook: $(TEMPLATES)

#
# checking for "redhat" style sysv scripts:
#

if USE_REDHAT

sysvdir	= /etc/rc.d/init.d
sysv_SCRIPTS = atalk

atalk: rc.atalk.redhat
	cp -f rc.atalk.redhat atalk
	chmod a+x atalk

endif

#
# checking for "SuSE" style sysv scripts:
#

if USE_SUSE

sysvdir	= /sbin/init.d
sysv_SCRIPTS = atalk

atalk: rc.atalk.suse
	cp -f rc.atalk.suse atalk
	chmod a+x atalk

endif

#
#
# checking for "cobalt" style sysv scripts:
#

if USE_COBALT

sysvdir	= /etc/rc.d/init.d
sysv_SCRIPTS = atalk

atalk: rc.atalk.cobalt
	cp -f rc.atalk.cobalt atalk
	chmod a+x atalk

endif

#
# checking for "tru64" style sysv scripts:
#

if USE_TRU64

sysvdir	= /sbin/init.d
sysv_SCRIPTS = atalk

atalk: rc.atalk.tru64
	cp -f rc.atalk.tru64 atalk
	chmod a+x atalk

endif
