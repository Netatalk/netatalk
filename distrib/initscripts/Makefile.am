## Makefile for distrib/initscripts/

SUFFIXES = .tmpl .

pkgconfdir = @PKGCONFDIR@
systemddir = @SYSTEMDDIR@

#
# Template Generation
#

.tmpl:
	sed -e s@:BINDIR:@${bindir}@ \
	    -e s@:SBINDIR:@${sbindir}@ \
	    -e s@:ETCDIR:@${pkgconfdir}@ \
	    -e s@:PKGLIBEXECDIR:@${pkglibexecdir}@ \
	    -e s@:NETATALK_VERSION:@${NETATALK_VERSION}@ \
	    <$< >$@

GENERATED_FILES = \
	a2boot.service		\
	afpd.service		\
	atalkd.service		\
	cnid.service		\
	io.netatalk.daemon.plist \
	netatalkd		\
	papd.service		\
	rc.a2boot.netbsd	\
	rc.afpd.netbsd		\
	rc.atalkd.netbsd	\
	rc.cnid_metad.netbsd	\
	rc.debian		\
	rc.openrc		\
	rc.papd.netbsd		\
	rc.redhat		\
	rc.solaris		\
	rc.suse			\
	rc.timelord.netbsd	\
	timelord.service

TEMPLATES = \
	a2boot.service.tmpl		\
	afpd.service.tmpl		\
	atalkd.service.tmpl		\
	cnid.service.tmpl		\
	io.netatalk.daemon.plist.tmpl	\
	netatalkd.tmpl			\
	papd.service.tmpl		\
	rc.a2boot.netbsd.tmpl		\
	rc.afpd.netbsd.tmpl		\
	rc.atalkd.netbsd.tmpl		\
	rc.cnid_metad.netbsd.tmpl	\
	rc.debian.tmpl			\
	rc.openrc.tmpl			\
	rc.papd.netbsd.tmpl		\
	rc.redhat.tmpl			\
	rc.solaris.tmpl			\
	rc.suse.tmpl			\
	rc.timelord.netbsd.tmpl		\
	timelord.service.tmpl

CLEANFILES = $(GENERATED_FILES) $(sysv_SCRIPTS) $(service_DATA) a2boot afpd atalkd cnid_metad papd timelord
EXTRA_DIST = $(TEMPLATES)

# overwrite automake uninstall
# not beautiful, but this way we can call the OS specific init script
# tools, like chkconfig, insserv or rc-update

uninstall: uninstall-startup

#
# checking for Red Hat style sysv scripts:
#

if USE_REDHAT

sysvdir	= /etc/rc.d/init.d
sysv_SCRIPTS = netatalk

$(sysv_SCRIPTS): rc.redhat
	cp -f rc.redhat $(sysv_SCRIPTS)
	chmod a+x $(sysv_SCRIPTS)

install-data-hook:
if USE_INSTALL_PRIVILEGED
	-chkconfig --add $(sysv_SCRIPTS)
endif

uninstall-startup:
if USE_INSTALL_PRIVILEGED
	-chkconfig --del $(sysv_SCRIPTS)
	rm -f $(DESTDIR)$(sysvdir)/$(sysv_SCRIPTS)
endif

endif

#
# checking for general systemd scripts:
#

if USE_SYSTEMD

servicedir	= $(systemddir)
service_DATA	= a2boot.service afpd.service atalkd.service cnid.service papd.service timelord.service

install-data-hook:
if USE_INSTALL_PRIVILEGED
	-systemctl daemon-reload
endif

uninstall-startup:
if USE_INSTALL_PRIVILEGED
	-systemctl disable $(service_DATA)
	rm -f $(DESTDIR)$(servicedir)/{a2boot,afpd,atalkd,cnid,papd,timelord}.service
	-systemctl daemon-reload
endif

endif

#
# checking for SUSE style sysv scripts:
#

if USE_SUSE

sysvdir	= /etc/init.d
sysv_SCRIPTS = netatalk

$(sysv_SCRIPTS): rc.suse
	cp -f rc.suse $(sysv_SCRIPTS)
	chmod a+x $(sysv_SCRIPTS)

install-data-hook:
if USE_INSTALL_PRIVILEGED
	-insserv $(sysv_SCRIPTS)
endif

uninstall-startup:
if USE_INSTALL_PRIVILEGED
	-insserv -d $(sysv_SCRIPTS)
	rm -f $(DESTDIR)$(sysvdir)/$(sysv_SCRIPTS)
endif

endif

#
# checking for NetBSD init scripts
#

if USE_NETBSD

sysvdir = /etc/rc.d
sysv_SCRIPTS = afpd atalkd cnid_metad papd timelord a2boot

afpd: rc.afpd.netbsd
	cp -f $? $@
	chmod a+x $@

atalkd: rc.atalkd.netbsd
	cp -f $? $@
	chmod a+x $@

cnid_metad: rc.cnid_metad.netbsd
	cp -f $? $@
	chmod a+x $@

papd: rc.papd.netbsd
	cp -f $? $@
	chmod a+x $@

timelord: rc.timelord.netbsd
	cp -f $? $@
	chmod a+x $@

a2boot: rc.a2boot.netbsd
	cp -f $? $@
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
if USE_INSTALL_PRIVILEGED
	rm -f $(DESTDIR)/etc/rc2.d/S90$(sysv_SCRIPTS)
	-ln -s ../init.d/$(sysv_SCRIPTS) $(DESTDIR)/etc/rc2.d/S90$(sysv_SCRIPTS)
	rm -f $(DESTDIR)/etc/rc0.d/K04$(sysv_SCRIPTS)
	-ln -s ../init.d/$(sysv_SCRIPTS) $(DESTDIR)/etc/rc0.d/K04$(sysv_SCRIPTS)
endif

uninstall-startup:
if USE_INSTALL_PRIVILEGED
	rm -f $(DESTDIR)$(sysvdir)/$(sysv_SCRIPTS) \
		$(DESTDIR)/etc/rc2.d/S90$(sysv_SCRIPTS) \
		$(DESTDIR)/etc/rc0.d/K04$(sysv_SCRIPTS)
endif

endif

#
# checking for openrc scripts (Gentoo, etc.):
#

if USE_OPENRC

sysvdir = /etc/init.d
sysv_SCRIPTS = netatalk

$(sysv_SCRIPTS): rc.openrc
	cp -f rc.openrc $(sysv_SCRIPTS)
	chmod a+x $(sysv_SCRIPTS)

install-data-hook:
if USE_INSTALL_PRIVILEGED
	-rc-update add $(sysv_SCRIPTS) default
endif

uninstall-startup:
if USE_INSTALL_PRIVILEGED
	-rc-update del $(sysv_SCRIPTS) default
	rm -f $(DESTDIR)$(sysvdir)/$(sysv_SCRIPTS)
endif

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
if USE_INSTALL_PRIVILEGED
	update-rc.d $(sysv_SCRIPTS) defaults 90 10
endif

uninstall-startup:
if USE_INSTALL_PRIVILEGED
	rm -f $(DESTDIR)$(sysvdir)/$(sysv_SCRIPTS)
	update-rc.d netatalk remove
endif

endif

#
# checking for macOS init script
#

if USE_MACOS

launchddir = /Library/LaunchDaemons
bin_SCRIPTS = netatalkd
launchd_DATA = io.netatalk.daemon.plist

install-data-hook:
if USE_INSTALL_PRIVILEGED
	(if [ ! -f $(launchddir)/$(launchd_DATA) ] ; then \
		cp $(launchd_DATA) $(launchddir) && \
		launchctl load -w $(launchddir)/$(launchd_DATA); \
	fi)
endif

uninstall-startup:
if USE_INSTALL_PRIVILEGED
	$(bin_SCRIPTS) stop
	launchctl unload -w $(launchddir)/$(launchd_DATA)
	rm -f $(DESTDIR)$(bindir)/$(bin_SCRIPTS)
	rm -f $(launchddir)/$(launchd_DATA)
endif

endif

#
# defaults, no init scripts installed
#

if USE_UNDEF

install-data-hook:

uninstall-hook:

uninstall-startup: uninstall-am

endif

