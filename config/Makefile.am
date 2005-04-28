## Makefile.am for config/

SUFFIXES = .tmpl .

GENFILES = afpd.conf AppleVolumes.default
TMPLFILES = afpd.conf.tmpl AppleVolumes.default.tmpl
CONFFILES = AppleVolumes.system \
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
	    -e s@:ETCDIR:@${pkgconfdir}@ \
	    -e s@:COMPILED_BACKENDS:@"$(compiled_backends)"@ \
	    -e s@:DEFAULT_CNID_SCHEME:@$(DEFAULT_CNID_SCHEME)@ \
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

pamdir = @PAMDIR@/etc/pam.d

install-data-local: install-config-files $(PAMFILES)
	$(mkinstalldirs) $(DESTDIR)$(pamdir)
	if test "x$(OVERWRITE_CONFIG)" = "xyes" -o ! -f $(DESTDIR)$(pamdir)/netatalk; then \
	    echo "$(INSTALL_DATA) $$f $(DESTDIR)$(pamdir)/netatalk"; \
	    $(INSTALL_DATA) netatalk.pamd $(DESTDIR)$(pamdir)/netatalk || echo "WARNING: Can't install PAM files";		\
	else \
	    echo "not overwriting $(DESTDIR)$(pamdir)/netatalk"; \
	fi; 

else

install-data-local: install-config-files

endif 
