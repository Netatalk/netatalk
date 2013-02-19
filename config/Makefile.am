## Makefile.am for config/

SUBDIRS = pam
SUFFIXES = .tmpl .

TMPLFILES = afp.conf.tmpl dbus-session.conf.tmpl
GENFILES = afp.conf dbus-session.conf
CLEANFILES = $(GENFILES)
EXTRA_DIST = afp.conf.tmpl extmap.conf dbus-session.conf.tmpl

OVERWRITE_CONFIG = @OVERWRITE_CONFIG@

CONFFILES = extmap.conf

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
	mkdir -pm 0755 $(DESTDIR)$(localstatedir)/netatalk/
	mkdir -pm 0755 $(DESTDIR)$(localstatedir)/netatalk/CNID/
	$(INSTALL_DATA) $(srcdir)/README $(DESTDIR)$(localstatedir)/netatalk/
	$(INSTALL_DATA) $(srcdir)/README $(DESTDIR)$(localstatedir)/netatalk/CNID/

uninstall-local:
	for f in $(CONFFILES) $(GENFILES); do \
		echo rm -f $(DESTDIR)$(pkgconfdir)/$$f; \
		rm -f $(DESTDIR)$(pkgconfdir)/$$f; \
	done
	rm -f $(DESTDIR)$(localstatedir)/netatalk/README
	rm -f $(DESTDIR)$(localstatedir)/netatalk/CNID/README
if HAVE_TRACKER_RDF
	rm -f $(DESTDIR)$(pkgconfdir)/tracker/tracker.cfg
endif
if USE_DEBIAN
	rm -f $(DESTDIR)/etc/default/netatalk
endif

install-config-files: $(CONFFILES) $(GENFILES)
	$(mkinstalldirs) $(DESTDIR)$(pkgconfdir)
	for f in $(CONFFILES) ; do \
		if test "x$(OVERWRITE_CONFIG)" = "xyes" -o ! -f $(DESTDIR)$(pkgconfdir)/$$f; then \
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
if HAVE_TRACKER_RDF
	if test "x$(OVERWRITE_CONFIG)" = "xyes" -o ! -f $(DESTDIR)$(pkgconfdir)/tracker/tracker.cfg ; then \
		$(INSTALL_DATA) tracker.cfg $(DESTDIR)$(pkgconfdir)/tracker/ ; \
	fi
endif
