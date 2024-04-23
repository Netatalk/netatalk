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
	    -e s@:PATH_NETATALK_LOCK:@${PATH_NETATALK_LOCK}@ \
	    <$< >$@

GENERATED_FILES = \
	io.netatalk.daemon.plist \
	netatalk.xml \
	netatalkd \
	rc.debian \
	rc.freebsd \
	rc.netbsd \
	rc.openbsd \
	rc.openrc \
	rc.redhat \
	rc.solaris \
	rc.suse \
	service.systemd

TEMPLATES = \
	io.netatalk.daemon.plist.tmpl \
	netatalk.xml.tmpl \
	netatalkd.tmpl \
	rc.debian.tmpl \
	rc.freebsd.tmpl \
	rc.netbsd.tmpl \
	rc.openbsd.tmpl \
	rc.openrc.tmpl \
	rc.redhat.tmpl \
	rc.solaris.tmpl \
	rc.suse.tmpl \
	service.systemd.tmpl

CLEANFILES = $(GENERATED_FILES) $(sysv_SCRIPTS) $(service_DATA) afpd cnid_metad
EXTRA_DIST = $(TEMPLATES)
noinst_DATA = $(GENERATED_FILES)

# overwrite automake uninstall
# not beautiful, but this way we can call the OS specific init script
# tools, like chkconfig, insserv or rc-update

uninstall: uninstall-startup

#
# checking for "redhat" style sysv scripts:
#

if USE_REDHAT_SYSV

sysvdir	= $(INIT_DIR)
sysv_SCRIPTS = netatalk

$(sysv_SCRIPTS): rc.redhat
	cp -f rc.redhat $(sysv_SCRIPTS)
	chmod a+x $(sysv_SCRIPTS)

install-data-hook:
if USE_INIT_HOOKS
	-chkconfig --add $(sysv_SCRIPTS)
endif

uninstall-startup:
if USE_INIT_HOOKS
	-chkconfig --del $(sysv_SCRIPTS)
	rm -f $(DESTDIR)$(sysvdir)/$(sysv_SCRIPTS)
endif

endif

#
# checking for general systemd scripts:
#

if USE_SYSTEMD

servicedir	= $(INIT_DIR)
service_DATA	= netatalk.service

netatalk.service: service.systemd
	cp -f service.systemd netatalk.service

install-data-hook:
if USE_INIT_HOOKS
	-systemctl daemon-reload
endif

uninstall-startup:
if USE_INIT_HOOKS
	-systemctl disable $(service_DATA)
	rm -f $(DESTDIR)$(servicedir)/netatalk.service
	-systemctl daemon-reload
endif

endif

#
# checking for "Debian" style systemd scripts:
#

if USE_DEBIAN_SYSTEMD

servicedir	= $(INIT_DIR)
service_DATA	= netatalk.service

netatalk.service: service.systemd
	cp -f service.systemd netatalk.service

install-data-hook:
if USE_INIT_HOOKS
	-systemctl daemon-reload
endif

uninstall-startup:
if USE_INIT_HOOKS
	-systemctl disable $(service_DATA)
	rm -f $(DESTDIR)$(servicedir)/netatalk.service
	-systemctl daemon-reload
endif

endif

#
# checking for "SuSE" style sysv scripts:
#

if USE_SUSE_SYSV

sysvdir	= $(INIT_DIR)
sysv_SCRIPTS = netatalk

$(sysv_SCRIPTS): rc.suse
	cp -f rc.suse $(sysv_SCRIPTS)
	chmod a+x $(sysv_SCRIPTS)

install-data-hook:
if USE_INIT_HOOKS
	-insserv $(sysv_SCRIPTS)
endif

uninstall-startup:
if USE_INIT_HOOKS
	-insserv -d $(sysv_SCRIPTS)
	rm -f $(DESTDIR)$(sysvdir)/$(sysv_SCRIPTS)
endif

endif

#
# checking for FreeBSD scripts:
#

if USE_FREEBSD

sysvdir = $(INIT_DIR)
sysv_SCRIPTS = netatalk

$(sysv_SCRIPTS): rc.freebsd
	cp -f rc.freebsd $(sysv_SCRIPTS)
	chmod a+x $(sysv_SCRIPTS)

install-data-hook:
if USE_INIT_HOOKS
	$(DESTDIR)$(sysvdir)/$(sysv_SCRIPTS) enable
endif

uninstall-startup:
if USE_INIT_HOOKS
	$(DESTDIR)$(sysvdir)/$(sysv_SCRIPTS) disable
	rm -f $(DESTDIR)$(sysvdir)/$(sysv_SCRIPTS)
endif

endif

#
# checking for NetBSD init scripts
#

if USE_NETBSD

sysvdir = $(INIT_DIR)
sysv_SCRIPTS = netatalk

netatalk: rc.netbsd
	cp -f $< $@
	chmod a+x $@

install-data-hook:

uninstall-hook:

uninstall-startup: uninstall-am

endif

#
# checking for OpenBSD scripts:
#

if USE_OPENBSD

sysvdir = $(INIT_DIR)
sysv_SCRIPTS = netatalk

$(sysv_SCRIPTS): rc.openbsd
	cp -f rc.openbsd $(sysv_SCRIPTS)
	chmod a+x $(sysv_SCRIPTS)

install-data-hook:
if USE_INIT_HOOKS
	-rcctl -d enable $(sysv_SCRIPTS)
endif

uninstall-startup:
if USE_INIT_HOOKS
	-rcctl -d disable $(sysv_SCRIPTS)
	rm -f $(DESTDIR)$(sysvdir)/$(sysv_SCRIPTS)
endif

endif

#
# checking for Solaris init scripts
#

if USE_SOLARIS

servicedir = $(INIT_DIR)
service_DATA = netatalk.xml

install-data-hook:
if USE_INIT_HOOKS
	svccfg import netatalk.xml
endif

uninstall-startup:
if USE_INIT_HOOKS
	svccfg delete network/netatalk
endif

endif

#
# checking for general openrc scripts:
#

if USE_OPENRC

sysvdir = $(INIT_DIR)
sysv_SCRIPTS = netatalk

$(sysv_SCRIPTS): rc.gentoo
	cp -f rc.gentoo $(sysv_SCRIPTS)
	chmod a+x $(sysv_SCRIPTS)

install-data-hook:
if USE_INIT_HOOKS
	-rc-update add $(sysv_SCRIPTS) default
endif

uninstall-startup:
if USE_INIT_HOOKS
	-rc-update del $(sysv_SCRIPTS) default
	rm -f $(DESTDIR)$(sysvdir)/$(sysv_SCRIPTS)
endif

endif

#
# checking for "Debian" style sysv scripts:
#

if USE_DEBIAN_SYSV

sysvdir = $(INIT_DIR)
sysv_SCRIPTS = netatalk

$(sysv_SCRIPTS): rc.debian
	cp -f rc.debian $(sysv_SCRIPTS)
	chmod a+x $(sysv_SCRIPTS)

install-data-hook:
if USE_INIT_HOOKS
	update-rc.d $(sysv_SCRIPTS) defaults 90 10
endif

uninstall-startup:
if USE_INIT_HOOKS
	rm -f $(DESTDIR)$(sysvdir)/$(sysv_SCRIPTS)
	update-rc.d netatalk remove
endif

endif

#
# checking for macOS init script
#

if USE_MACOS_LAUNCHD

bin_SCRIPTS = netatalkd
launchd_PLIST = io.netatalk.daemon.plist

install-data-hook:
if USE_INIT_HOOKS
	(if [ ! -f $(INIT_DIR)/$(launchd_PLIST) ] ; then \
		cp $(launchd_PLIST) $(INIT_DIR) && \
		launchctl load -w $(INIT_DIR)/$(launchd_PLIST); \
	fi)
endif

uninstall-startup:
if USE_INIT_HOOKS
	$(bin_SCRIPTS) stop
	launchctl unload -w $(INIT_DIR)/$(launchd_PLIST)
	rm -f $(DESTDIR)$(bindir)/$(bin_SCRIPTS)
	rm -f $(INIT_DIR)/$(launchd_PLIST)
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

