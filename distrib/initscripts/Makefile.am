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
	rc.afpd.netbsd		\
	rc.atalk.redhat		\
	rc.atalk.tru64		\
	rc.atalk.bsd		\
	rc.atalkd.netbsd	\
	rc.atalk.suse		\
	rc.papd.netbsd		\
	rc.timelord.netbsd
TEMPLATES = $(foreach f,$(GENERATED_FILES),$(f).tmpl)

CLEANFILES = $(GENERATED_FILES)
EXTRA_DIST = $(TEMPLATES) rc.atalk.cobalt rc.atalk.sysv

install-data-hook: $(GENERATED_FILES)

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

sysvdir	= /etc/rc.d
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

#
# checking for NetBSD init scripts
#

if USE_NETBSD

sysvdir = /etc/rc.d
sysv_SCRIPTS = afp atalk pap timelord

afpd: rc.afpd.netbsd
	cp -f $< $@

atalkd: rc.atalkd.netbsd
	cp -f $< $@
	chmod a+x $@

papd: rc.papd.netbsd
	cp -f $< $@
	chmod a+x $@

timelord: rc.timelord.netbsd
	cp -f $< $@
	chmod a+x $@

endif

