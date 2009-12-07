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
	rc.cnid_metad.netbsd	\
	rc.papd.netbsd		\
	rc.timelord.netbsd	\
	rc.atalk.sysv		\
	rc.atalk.gentoo		\
	rc.atalk.debian

TEMPLATES = \
	rc.afpd.netbsd.tmpl		\
	rc.atalk.redhat.tmpl		\
	rc.atalk.tru64.tmpl		\
	rc.atalk.bsd.tmpl		\
	rc.atalkd.netbsd.tmpl		\
	rc.atalk.suse.tmpl		\
	rc.cnid_metad.netbsd.tmpl	\
	rc.papd.netbsd.tmpl		\
	rc.timelord.netbsd.tmpl		\
	rc.atalk.sysv.tmpl		\
	rc.atalk.gentoo.tmpl		\
	rc.atalk.debian.tmpl

CLEANFILES = $(GENERATED_FILES) $(sysv_SCRIPTS) afpd atalkd cnid_metad papd timelord
EXTRA_DIST = $(TEMPLATES) rc.atalk.cobalt

# overwrite automake uninstall
# not beautiful, but this way we can call the OS specific init script
# tools, like chkconfig, insserv or rc-update

uninstall: uninstall-startup

#
# checking for "redhat" style sysv scripts:
#

if USE_REDHAT

sysvdir	= $(DESTDIR)/etc/rc.d/init.d
sysv_SCRIPTS = atalk

$(sysv_SCRIPTS): rc.atalk.redhat
	cp -f rc.atalk.redhat $(sysv_SCRIPTS)
	chmod a+x $(sysv_SCRIPTS)

install-data-hook:
	-chkconfig --add $(sysv_SCRIPTS)

uninstall-startup:
	-chkconfig --del $(sysv_SCRIPTS)
	rm -f $(sysvdir)/$(sysv_SCRIPTS)

endif

#
# checking for "SuSE" style sysv scripts:
#

if USE_SUSE

sysvdir	= $(DESTDIR)/etc/init.d
sysv_SCRIPTS = atalk

$(sysv_SCRIPTS): rc.atalk.suse
	cp -f rc.atalk.suse $(sysv_SCRIPTS)
	chmod a+x $(sysv_SCRIPTS)

install-data-hook:
	-insserv $(sysv_SCRIPTS)

uninstall-startup:
	-insserv -d $(sysv_SCRIPTS)
	rm -f $(sysvdir)/$(sysv_SCRIPTS)

endif

#
# checking for "cobalt" style sysv scripts:
#

if USE_COBALT

sysvdir	= $(DESTDIR)/etc/rc.d/init.d
sysv_SCRIPTS = atalk

$(sysv_SCRIPTS): rc.atalk.cobalt
	cp -f rc.atalk.cobalt $(sysv_SCRIPTS)
	chmod a+x $(sysv_SCRIPTS)

install-data-hook:

uninstall-hook:

uninstall-startup: uninstall-am

endif

#
# checking for "tru64" style sysv scripts:
#

if USE_TRU64

sysvdir	= $(DESTDIR)/etc/init.d
sysv_SCRIPTS = atalk

$(sysv_SCRIPTS): rc.atalk.tru64
	cp -f rc.atalk.tru64 $(sysv_SCRIPTS)
	chmod a+x $(sysv_SCRIPTS)

install-data-hook:

uninstall-hook:

uninstall-startup: uninstall-am

endif

#
# checking for NetBSD init scripts
#

if USE_NETBSD

sysvdir = $(DESTDIR)/etc/rc.d
sysv_SCRIPTS = afpd atalkd cnid_metad papd timelord

afpd: rc.afpd.netbsd
	cp -f $< $@
	chmod a+x $@

atalkd: rc.atalkd.netbsd
	cp -f $< $@
	chmod a+x $@

cnid_metad: rc.cnid_metad.netbsd
	cp -f $< $@
	chmod a+x $@

papd: rc.papd.netbsd
	cp -f $< $@
	chmod a+x $@

timelord: rc.timelord.netbsd
	cp -f $< $@
	chmod a+x $@

install-data-hook:

uninstall-hook:

uninstall-startup: uninstall-am

endif

#
# checking for Solaris init scripts
#

if USE_SOLARIS

sysvdir = $(DESTDIR)/etc/init.d
sysv_SCRIPTS = netatalk

$(sysv_SCRIPTS): rc.atalk.sysv
	cp -f rc.atalk.sysv $@
	chmod a+x $@

install-data-hook:
	rm -f $(DESTDIR)/etc/rc2.d/S90$(sysv_SCRIPTS)
	-ln -s ../init.d/$(sysv_SCRIPTS) $(DESTDIR)/etc/rc2.d/S90$(sysv_SCRIPTS)
	rm -f $(DESTDIR)/etc/rc0.d/K04$(sysv_SCRIPTS)
	-ln -s ../init.d/$(sysv_SCRIPTS) $(DESTDIR)/etc/rc0.d/K04$(sysv_SCRIPTS)

uninstall-startup:
	rm -f $(sysvdir)/$(sysv_SCRIPTS) \
		$(DESTDIR)/etc/rc2.d/S90$(sysv_SCRIPTS) \
		$(DESTDIR)/etc/rc0.d/K04$(sysv_SCRIPTS)

endif

#
# checking for "Gentoo" style sysv scripts:
#

if USE_GENTOO

sysvdir = $(DESTDIR)/etc/init.d
sysv_SCRIPTS = atalk

$(sysv_SCRIPTS): rc.atalk.gentoo
	cp -f rc.atalk.gentoo $(sysv_SCRIPTS)
	chmod a+x $(sysv_SCRIPTS)

install-data-hook:
	-rc-update add $(sysv_SCRIPTS) default

uninstall-startup:
	-rc-update del $(sysv_SCRIPTS) default
	rm -f $(sysvdir)/$(sysv_SCRIPTS)

endif

#
# checking for "Debian" style sysv scripts:
#

if USE_DEBIAN

sysvdir = $(DESTDIR)/etc/init.d
sysv_SCRIPTS = netatalk

$(sysv_SCRIPTS): rc.atalk.debian
	cp -f rc.atalk.debian $(sysv_SCRIPTS)
	chmod a+x $(sysv_SCRIPTS)

install-data-hook:
#	update-rc.d $(sysv_SCRIPTS) defaults 90 10

uninstall-startup:
#	rm -f $(sysvdir)/$(sysv_SCRIPTS)
#	update-rc.d netatalk remove

endif


#
# defaults, no init scripts installed
#

if USE_UNDEF

install-data-hook:

uninstall-hook:

uninstall-startup: uninstall-am

endif

