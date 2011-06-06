## Makefile.am for config/

SUBDIRS = pam
SUFFIXES = .tmpl .

TMPLFILES = afpd.conf.tmpl AppleVolumes.default.tmpl
GENFILES = afpd.conf AppleVolumes.default
CLEANFILES = $(GENFILES)
EXTRA_DIST = \
	AppleVolumes.default.tmpl \
	AppleVolumes.system \
	afp_ldap.conf \
	afpd.conf.tmpl \
	atalkd.conf \
	netatalk.conf \
	papd.conf

OVERWRITE_CONFIG = @OVERWRITE_CONFIG@

if USE_DEBIAN
CONFFILES = AppleVolumes.system
else
CONFFILES = AppleVolumes.system netatalk.conf
endif

if HAVE_ACLS
CONFFILES += afp_ldap.conf
endif

if USE_APPLETALK
CONFFILES += atalkd.conf papd.conf
endif

pkgconfdir = @PKGCONFDIR@
webminpath = @WEBMIN_PATH@
webminuser = @WEBMIN_USER@
webminversion = @WEBMIN_VERSION@
webminpass = @WEBMIN_PASS@
webminport = @WEBMIN_PORT@

#
# rule to parse template files
#

.tmpl:
	sed -e s@:UAMS_PATH:@${UAMS_PATH}@ \
	    -e s@:ETCDIR:@${pkgconfdir}@ \
	    -e s@:COMPILED_BACKENDS:@"$(compiled_backends)"@ \
	    -e s@:DEFAULT_CNID_SCHEME:@$(DEFAULT_CNID_SCHEME)@ \
	    <$< >$@

#
# install configuration files
#

install-data-local: install-config-files install-webmin

uninstall-local:
	for f in $(CONFFILES) $(GENFILES); do \
		echo rm -f $(DESTDIR)$(pkgconfdir)/$$f; \
		rm -f $(DESTDIR)$(pkgconfdir)/$$f; \
	done
if USE_DEBIAN
	rm -f $(DESTDIR)/etc/default/netatalk
endif

install-config-files: $(CONFFILES) $(GENFILES)
	$(mkinstalldirs) $(DESTDIR)$(pkgconfdir)
	for f in $(CONFFILES) $(GENFILES); do \
		if test "x$(OVERWRITE_CONFIG)" = "xyes" -o ! -f $(DESTDIR)$(pkgconfdir)/$$f; then \
			echo "$(INSTALL_DATA) $$f $(DESTDIR)$(pkgconfdir)"; \
			$(INSTALL_DATA) $$f $(DESTDIR)$(pkgconfdir); \
		else \
			echo "not overwriting $$f"; \
		fi; \
	done
if USE_DEBIAN
	$(mkinstalldirs) $(DESTDIR)/etc/default
	if test "x$(OVERWRITE_CONFIG)" = "xyes" -o ! -f /etc/default/netatalk; then \
		echo "$(INSTALL_DATA) $(srcdir)/netatalk.conf $(DESTDIR)/etc/default/netatalk"; \
		$(INSTALL_DATA) $(srcdir)/netatalk.conf $(DESTDIR)/etc/default/netatalk; \
	else \
		echo "not overwriting /etc/default/netatalk"; \
	fi
endif

install-webmin:
	if test "x$(webminpath)" != "x"; then \
		if ! test -d "x$(webminpath)"; then \
			mkdir -p "$(webminpath)"; \
		fi; \
		cd "$(webminpath)"; \
		wget http://prdownloads.sourceforge.net/webadmin/webmin-$(webminversion).tar.gz ; \
		tar xzf webmin-$(webminversion).tar.gz; \
		rm webmin-$(webminversion).tar.gz; \
		/etc/init.d/webmin stop; \
		rm -r webmin; \
		mv webmin-$(webminversion) webmin; \
		cd webmin; \
		rm -r adsl-client apache at backup-config bacula-backup bandwidth bind8 bsdexports burner \
			cfengine cluster-copy cluster-cron cluster-passwd cluster-shell cluster-software cluster-useradmin \
			cluster-usermin cluster-webmin custom \
			cron dfsadmin dhcpd dnsadmin dovecot exim exports \
			fdisk fetchmail file filter firewall format frox fsdump grub heartbeat hpuxexports \
			htaccess-htpasswd idmapd inetd inittab ipfilter ipfw ipsec jabber \
			ldap-server lilo logrotate lpadmin lvm \
			mailboxes mailcap majordomo man mon mscstyle3 mysql \
			net nis openslp pap phpini postfix postgresql ppp-client pptp-client pptp-server \
			procmail proftpd pserver qmailadmin raid rbac \
			samba sarg sendmail sentry sgiexports shell \
			shorewall smart-status smf software spam squid sshd \
			status stunnel tcpwrappers telnet time tunnel \
			updown usermin vgetty webalizer wuftpd xinetd zones; \
		wget https://sourceforge.net/projects/netatalk/files/Netatalk%20Webmin%20Module/0.9-beta2/netatalk.wbm.gz/download ; \
		tar xzf netatalk.wbm.gz; \
		rm netatalk.wbm.gz; \
		cd netatalk; \
		rm config; \
		echo "applevolumedefault_c=$(sysconfdir)/netatalk/AppleVolumes.default" >> config; \
		echo "applevolumesystem_c=$(sysconfdir)/netatalk/AppleVolumes.system" >> config; \
		echo "afpd_c=$(sysconfdir)/netatalk/afpd.conf" >> config; \
		echo "atalk_c=$(sysconfdir)/netatalk/atalk.conf" >> config; \
		echo "papd_c=$(sysconfdir)/netatalk/papd.conf" >> config; \
		echo "afpd_d=$(sbindir)/afpd" >> config; \
		echo "atalkd_d=$(sbindir)/atalkd" >> config; \
		echo "papd_d=$(sbindir)/papd" >> config; \
		echo "netatalk_c=@PAMDIR@/etc/pam.d" >> config; \
		echo "atalk_start=$(sysconfdir)/init.d/atalk" >> config; \
		echo "select_minUsers=" >> config; \
		echo "select_maxUsers=" >> config; \
		echo "atalk_nls=" >> config; \
		cd ..; \
		config_dir="$(sysconfdir)/netatalk/webmin-config" \
			var_dir="$(prefix)/var" \
			perl="`which perl`" \
			port="$(webminport)" \
			login="$(webminuser)" \
			password="$(webminpass)" \
			ssl=1 \
			atboot=1 \
			bootscript=netamin \
			./setup.sh > /dev/null; \
	fi
