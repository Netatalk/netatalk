## Makefile.am for config/

SUFFIXES = .tmpl .

GENFILES = afpd.conf
TMPLFILES = $(foreach file,$(GENFILES),$(file).tmpl)
CONFFILES = AppleVolumes.default AppleVolumes.system \
	atalkd.conf netatalk.conf papd.conf
PAMFILES = netatalk.pamd

EXTRA_DIST = $(CONFFILES) $(TMPLFILES) $(PAMFILES)

CLEANFILES += $(GENFILES)

pkgconf_DATA = @OVERWRITE@
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
# pluggable authentication modules support
#

if USE_PAM

pamdir   = $(sysconfdir)/pam.d

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(pamdir)
	$(INSTALL_DATA) netatalk.pamd $(DESTDIR)$(pamdir)/netatalk

else

install-data-local:

endif 
