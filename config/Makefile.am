## Makefile.am for config/

SUFFIXES = .tmpl .

GENFILES = afpd.conf
TMPLFILES = $(foreach file,$(GENFILES),$(file).tmpl)
CONFFILES = AppleVolumes.default AppleVolumes.system \
	atalkd.conf netatalk.conf papd.conf
PAMFILES = netatalk.pamd
OVERWRITE_CONFIG = @OVERWRITE_CONFIG@

EXTRA_DIST = $(CONFFILES) $(TMPLFILES) $(PAMFILES)

CLEANFILES = $(GENFILES)

pkgconfdir = @PKGCONFDIR@

#
# rule to parse template files
#

.tmpl:
	sed -e s@:UAMS_PATH:@${UAMS_PATH}@ \
	    -e s@:NLS_PATH:@${pkgconfdir}/nls@ \
	    -e s@:ETCDIR:@${pkgconfdir}@ \
	    <$< >$@

#
# install configuration files
#

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

#
# pluggable authentication modules support
#

if USE_PAM

pamdir = $(sysconfdir)/pam.d

install-data-local: install-config-files
	$(mkinstalldirs) $(DESTDIR)$(pamdir)
	$(INSTALL_DATA) netatalk.pamd $(DESTDIR)$(pamdir)/netatalk

else

install-data-local: install-config-files

endif 
