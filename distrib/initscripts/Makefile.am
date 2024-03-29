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
	rc.atalk.debian		\
	rc.atalk.gentoo		\
	rc.atalk.redhat-sysv	\
	rc.atalk.suse-sysv	\
	rc.atalk.systemd	\
	rc.atalk.sysv		\
	rc.atalkd.netbsd	\
	rc.cnid_metad.netbsd	\
	rc.papd.netbsd		\
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
	rc.atalk.debian.tmpl		\
	rc.atalk.gentoo.tmpl		\
	rc.atalk.redhat-sysv.tmpl	\
	rc.atalk.suse-sysv.tmpl		\
	rc.atalk.systemd.tmpl		\
	rc.atalk.sysv.tmpl		\
	rc.atalkd.netbsd.tmpl		\
	rc.cnid_metad.netbsd.tmpl	\
	rc.papd.netbsd.tmpl		\
	rc.timelord.netbsd.tmpl		\
	timelord.service.tmpl

CLEANFILES = $(GENERATED_FILES) $(sysv_SCRIPTS) $(service_DATA) a2boot afpd atalkd cnid_metad papd timelord
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

$(sysv_SCRIPTS): rc.atalk.redhat-sysv
	cp -f rc.atalk.redhat-sysv $(sysv_SCRIPTS)
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

servicedir	= $(systemddir)
service_DATA	= a2boot.service afpd.service atalkd.service cnid.service papd.service timelord.service

install-data-hook:
if USE_INSTALL_PRIVILEGED
	-systemctl daemon-reload
endif

uninstall-startup:
if USE_INSTALL_PRIVILEGED
	-systemctl disable $(service_DATA)
endif
	rm -f $(DESTDIR)$(servicedir)/{a2boot,afpd,atalkd,cnid,papd,timelord}.service
if USE_INSTALL_PRIVILEGED
	-systemctl daemon-reload
endif

endif

#
# checking for "SuSE" style sysv scripts:
#

if USE_SUSE_SYSV

sysvdir	= /etc/init.d
sysv_SCRIPTS = netatalk

$(sysv_SCRIPTS): rc.atalk.suse-sysv
	cp -f rc.atalk.suse-sysv $(sysv_SCRIPTS)
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

$(sysv_SCRIPTS): rc.atalk.sysv
	cp -f rc.atalk.sysv $@
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

$(sysv_SCRIPTS): rc.atalk.gentoo
	cp -f rc.atalk.gentoo $(sysv_SCRIPTS)
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

$(sysv_SCRIPTS): rc.atalk.debian
	cp -f rc.atalk.debian $(sysv_SCRIPTS)
	chmod a+x $(sysv_SCRIPTS)

install-data-hook:
#	update-rc.d $(sysv_SCRIPTS) defaults 90 10

uninstall-startup:
#	rm -f $(DESTDIR)$(sysvdir)/$(sysv_SCRIPTS)
#	update-rc.d netatalk remove

endif

#
# checking for macOS init script
#

if USE_MACOS

launchddir = /Library/LaunchDaemons
bin_SCRIPTS = netatalkd
launchd_DATA = io.netatalk.daemon.plist

install-data-hook:
	(if [ ! -f $(launchddir)/$(launchd_DATA) ] ; then \
		cp $(launchd_DATA) $(launchddir) && \
		launchctl load -w $(launchddir)/$(launchd_DATA); \
	fi)

uninstall-startup:
	$(bin_SCRIPTS) stop
	launchctl unload -w $(launchddir)/$(launchd_DATA)
	rm -f $(DESTDIR)$(bindir)/$(bin_SCRIPTS)
	rm -f $(launchddir)/$(launchd_DATA)

endif

#
# defaults, no init scripts installed
#

if USE_UNDEF

install-data-hook:

uninstall-hook:

uninstall-startup: uninstall-am

endif

