## Makefile for distrib/pam/

SUFFIXES = .tmpl .
pamdir = @PAMDIR@/etc/pam.d
EXTRA_DIST = netatalk.pam.tmpl
noinst_SCRIPTS = netatalk.pam
CLEANFILES = netatalk.pam

.tmpl:
	sed -e "s,[@]PAM_DIRECTIVE[@],${PAM_DIRECTIVE},g" \
		-e "s,[@]PAM_AUTH[@],${PAM_AUTH}," \
		-e "s,[@]PAM_ACCOUNT[@],${PAM_ACCOUNT}," \
		-e "s,[@]PAM_PASSWORD[@],${PAM_PASSWORD}," \
		-e "s,[@]PAM_SESSION[@],${PAM_SESSION}," \
	    <$< >$@

if USE_PAM
install-data-local: netatalk.pam
	$(mkinstalldirs) $(DESTDIR)$(pamdir)
	if test "x$(OVERWRITE_CONFIG)" = "xyes" -o ! -f $(DESTDIR)$(pamdir)/netatalk; then \
	    echo "$(INSTALL_DATA) $$f $(DESTDIR)$(pamdir)/netatalk"; \
	    $(INSTALL_DATA) netatalk.pam $(DESTDIR)$(pamdir)/netatalk || echo "WARNING: Can't install PAM files";		\
	else \
	    echo "not overwriting $(DESTDIR)$(pamdir)/netatalk"; \
	fi; 

uninstall-local:
	echo rm -f $(DESTDIR)$(pamdir)/netatalk; \
	rm -f $(DESTDIR)$(pamdir)/netatalk; \
	for f in $(CONFFILES) $(GENFILES); do \
		echo rm -f $(DESTDIR)$(pkgconfdir)/$$f; \
		rm -f $(DESTDIR)$(pkgconfdir)/$$f; \
	done
else
install-data-local:
uninstall-local:
endif
