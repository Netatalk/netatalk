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

if HAVE_LDAP
CONFFILES += afp_ldap.conf
endif

if USE_APPLETALK
CONFFILES += atalkd.conf papd.conf
endif

pkgconfdir = @PKGCONFDIR@

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

install-data-local: install-config-files

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
	for f in $(CONFFILES); do \
		if test -f $(srcdir)$$f; then \
			echo "origin file not found $(srcdir)$$f"; \
			exit 1; \
		elif test "x$(OVERWRITE_CONFIG)" = "xyes" -o ! -f $(DESTDIR)$(pkgconfdir)/$$f; then \
			echo "$(INSTALL_DATA) $$f $(DESTDIR)$(pkgconfdir)"; \
			$(INSTALL_DATA) $(srcdir)/$$f $(DESTDIR)$(pkgconfdir); \
		else \
			echo "not overwriting $$f"; \
		fi; \
	done
	for f in $(GENFILES); do \
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
