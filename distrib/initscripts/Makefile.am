## Makefile for distrib/initscripts/

SUFFIXES = .tmpl .

pkgconfdir = @PKGCONFDIR@
systemddir = /lib/systemd/system

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
	rc.afpd.netbsd		\
	rc.atalk.redhat-sysv	\
	rc.atalk.systemd	\
	rc.atalk.tru64		\
	rc.atalk.bsd		\
	rc.atalkd.netbsd	\
	rc.atalk.suse-sysv	\
	rc.cnid_metad.netbsd	\
	rc.papd.netbsd		\
	rc.timelord.netbsd	\
	rc.atalk.sysv		\
	rc.atalk.gentoo		\
	rc.atalk.debian		\
	service.netatalk.systemd	\
	service.cnid.systemd	\
	service.atalkd.systemd	\
	service.papd.systemd	\
	service.timelord.systemd	\
	service.a2boot.systemd

TEMPLATES = \
	rc.afpd.netbsd.tmpl		\
	rc.atalk.redhat-sysv.tmpl	\
	rc.atalk.systemd.tmpl		\
	rc.atalk.tru64.tmpl		\
	rc.atalk.bsd.tmpl		\
	rc.atalkd.netbsd.tmpl		\
	rc.atalk.suse-sysv.tmpl		\
	rc.cnid_metad.netbsd.tmpl	\
	rc.papd.netbsd.tmpl		\
	rc.timelord.netbsd.tmpl		\
	rc.atalk.sysv.tmpl		\
	rc.atalk.gentoo.tmpl		\
	rc.atalk.debian.tmpl		\
	service.netatalk.systemd.tmpl	\
	service.cnid.systemd.tmpl	\
	service.atalkd.systemd.tmpl	\
	service.papd.systemd.tmpl	\
	service.timelord.systemd.tmpl	\
	service.a2boot.systemd.tmpl

CLEANFILES = $(GENERATED_FILES) $(sysv_SCRIPTS) $(service_netatalk_DATA) $(service_cnid_DATA) $(service_atalkd_DATA) $(service_papd_DATA) $(service_timelord_DATA) $(service_a2boot_DATA) afpd atalkd cnid_metad papd timelord
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

service_netatalkdir	= $(systemddir)
service_netatalk_DATA	= netatalk.service

$(service_netatalk_DATA): service.netatalk.systemd
	cp -f service.netatalk.systemd $(service_netatalk_DATA)

install-data-hook:
	-systemctl daemon-reload
	-systemctl enable $(service_netatalk_DATA)

uninstall-startup:
	-systemctl disable $(service_netatalk_DATA)
	rm -f $(DESTDIR)$(service_netatalkdir)/$(service_netatalk_DATA)
	-systemctl daemon-reload

service_cniddir	= $(systemddir)
service_cnid_DATA	= netatalk-cnid.service

$(service_cnid_DATA): service.cnid.systemd
	cp -f service.cnid.systemd $(service_cnid_DATA)

install-data-hook:
	-systemctl daemon-reload
	-systemctl enable $(service_cnid_DATA)

uninstall-startup:
	-systemctl disable $(service_cnid_DATA)
	rm -f $(DESTDIR)$(service_cniddir)/$(service_cnid_DATA)
	-systemctl daemon-reload

if USE_APPLETALK

service_atalkddir	= $(systemddir)
service_atalkd_DATA	= netatalk-atalkd.service

$(service_atalkd_DATA): service.atalkd.systemd
	cp -f service.atalkd.systemd $(service_atalkd_DATA)

install-data-hook:
	-systemctl daemon-reload
	-systemctl enable $(service_atalkd_DATA)

uninstall-startup:
	-systemctl disable $(service_atalkd_DATA)
	rm -f $(DESTDIR)$(service_atalkddir)/$(service_atalkd_DATA)
	-systemctl daemon-reload

endif

if USE_CUPS

service_papddir	= $(systemddir)
service_papd_DATA	= netatalk-papd.service

$(service_papd_DATA): service.papd.systemd
	cp -f service.papd.systemd $(service_papd_DATA)

install-data-hook:
	-systemctl daemon-reload
	-systemctl enable $(service_papd_DATA)

uninstall-startup:
	-systemctl disable $(service_papd_DATA)
	rm -f $(DESTDIR)$(service_papddir)/$(service_papd_DATA)
	-systemctl daemon-reload

endif

if COMPILE_TIMELORD

service_timelorddir	= $(systemddir)
service_timelord_DATA	= netatalk-timelord.service

$(service_timelord_DATA): service.timelord.systemd
	cp -f service.timelord.systemd $(service_timelord_DATA)

install-data-hook:
	-systemctl daemon-reload
	-systemctl enable $(service_timelord_DATA)

uninstall-startup:
	-systemctl disable $(service_timelord_DATA)
	rm -f $(DESTDIR)$(service_timelorddir)/$(service_timelord_DATA)
	-systemctl daemon-reload

endif

if COMPILE_A2BOOT

service_a2bootdir	= $(systemddir)
service_a2boot_DATA	= netatalk-a2boot.service

$(service_a2boot_DATA): service.a2boot.systemd
	cp -f service.a2boot.systemd $(service_a2boot_DATA)

install-data-hook:
	-systemctl daemon-reload
	-systemctl enable $(service_a2boot_DATA)

uninstall-startup:
	-systemctl disable $(service_a2boot_DATA)
	rm -f $(DESTDIR)$(service_a2bootdir)/$(service_a2boot_DATA)
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
# checking for "tru64" style sysv scripts:
#

if USE_TRU64

sysvdir	= /etc/init.d
sysv_SCRIPTS = netatalk

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

sysvdir = /etc/rc.d
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
# defaults, no init scripts installed
#

if USE_UNDEF

install-data-hook:

uninstall-hook:

uninstall-startup: uninstall-am

endif

