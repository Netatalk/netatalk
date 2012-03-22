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
	    -e s@:NETATALK_VERSION:@${NETATALK_VERSION}@ \
	    <$< >$@

GENERATED_FILES = \
	rc.bsd \
	rc.debian \
	rc.gentoo \
	rc.netbsd \
	rc.redhat \
	rc.solaris \
	rc.suse \
	service.netatalk.systemd

TEMPLATES = \
	rc.bsd.tmpl \
	rc.debian.tmpl \
	rc.gentoo.tmpl \
	rc.netbsd.tmpl \
	rc.redhat.tmpl \
	rc.solaris.tmpl \
	rc.suse.tmpl \
	service.netatalk.systemd.tmpl

CLEANFILES = $(GENERATED_FILES) $(sysv_SCRIPTS) $(service_DATA) afpd cnid_metad
EXTRA_DIST = $(TEMPLATES)

# overwrite automake uninstall
# not beautiful, but this way we can call the OS specific init script
# tools, like chkconfig, insserv or rc-update

uninstall: uninstall-startup

#
# checking for "redhat" style sysv scripts:
#

if USE_REDHAT_SYSV

sysvdir	= /etc/rc.d/init.d
sysv_SCRIPTS = netatalk

$(sysv_SCRIPTS): rc.redhat
	cp -f rc.redhat $(sysv_SCRIPTS)
	chmod a+x $(sysv_SCRIPTS)

install-data-hook:
	-chkconfig --add $(sysv_SCRIPTS)

uninstall-startup:
	-chkconfig --del $(sysv_SCRIPTS)
	rm -f $(DESTDIR)$(sysvdir)/$(sysv_SCRIPTS)

endif

#
# checking for general systemd scripts:
#

if USE_SYSTEMD

servicedir	= /lib/systemd/system
service_DATA	= netatalk.service

netatalk.service: service.netatalk.systemd
	cp -f service.netatalk.systemd netatalk.service

install-data-hook:
	-systemctl daemon-reload

uninstall-startup:
	-systemctl disable $(service_DATA)
	rm -f $(DESTDIR)$(servicedir)/netatalk.service
	-systemctl daemon-reload

endif

#
# checking for "SuSE" style sysv scripts:
#

if USE_SUSE_SYSV

sysvdir	= /etc/init.d
sysv_SCRIPTS = netatalk

$(sysv_SCRIPTS): rc.suse
	cp -f rc.suse $(sysv_SCRIPTS)
	chmod a+x $(sysv_SCRIPTS)

install-data-hook:
	-insserv $(sysv_SCRIPTS)

uninstall-startup:
	-insserv -d $(sysv_SCRIPTS)
	rm -f $(DESTDIR)$(sysvdir)/$(sysv_SCRIPTS)

endif

#
# checking for NetBSD init scripts
#

if USE_NETBSD

sysvdir = /etc/rc.d
sysv_SCRIPTS = netatalk

netatalk: rc.netbsd
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

sysvdir = /etc/init.d
sysv_SCRIPTS = netatalk

$(sysv_SCRIPTS): rc.solaris
	cp -f rc.solaris $@
	chmod a+x $@

install-data-hook:
	rm -f $(DESTDIR)/etc/rc2.d/S90$(sysv_SCRIPTS)
	-ln -s ../init.d/$(sysv_SCRIPTS) $(DESTDIR)/etc/rc2.d/S90$(sysv_SCRIPTS)
	rm -f $(DESTDIR)/etc/rc0.d/K04$(sysv_SCRIPTS)
	-ln -s ../init.d/$(sysv_SCRIPTS) $(DESTDIR)/etc/rc0.d/K04$(sysv_SCRIPTS)

uninstall-startup:
	rm -f $(DESTDIR)$(sysvdir)/$(sysv_SCRIPTS) \
		$(DESTDIR)/etc/rc2.d/S90$(sysv_SCRIPTS) \
		$(DESTDIR)/etc/rc0.d/K04$(sysv_SCRIPTS)

endif

#
# checking for "Gentoo" style sysv scripts:
#

if USE_GENTOO

sysvdir = /etc/init.d
sysv_SCRIPTS = netatalk

$(sysv_SCRIPTS): rc.gentoo
	cp -f rc.gentoo $(sysv_SCRIPTS)
	chmod a+x $(sysv_SCRIPTS)

install-data-hook:
#	-rc-update add $(sysv_SCRIPTS) default

uninstall-startup:
#	-rc-update del $(sysv_SCRIPTS) default
#	rm -f $(DESTDIR)$(sysvdir)/$(sysv_SCRIPTS)

endif

#
# checking for "Debian" style sysv scripts:
#

if USE_DEBIAN

sysvdir = /etc/init.d
sysv_SCRIPTS = netatalk

$(sysv_SCRIPTS): rc.debian
	cp -f rc.debian $(sysv_SCRIPTS)
	chmod a+x $(sysv_SCRIPTS)

install-data-hook:
#	update-rc.d $(sysv_SCRIPTS) defaults 90 10

uninstall-startup:
#	rm -f $(DESTDIR)$(sysvdir)/$(sysv_SCRIPTS)
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

