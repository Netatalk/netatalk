## Makefile.am for config/

SUFFIXES = .tmpl .

GENFILES = afpd.conf
TMPLFILES = $(foreach file,$(GENFILES),$(file).tmpl)
CONFFILES = AppleVolumes.default AppleVolumes.system \
	atalkd.conf netatalk.conf netatalk.pamd papd.conf
PAMFILES = netatalk.pamd

EXTRA_DIST = $(CONFFILES) $(TMPLFILES) $(PAMFILES)

CLEANFILES += $(GENFILES)

sysconf_DATA = $(CONFFILES) $(GENFILES)

#
# rule to parse template files
#

.tmpl:
	sed -e s@:UAMS_PATH:@${UAMS_PATH}@ \
	    -e s@:NLS_PATH:@${sysconfdir}/nls@ \
	    -e s@:ETCDIR:@${sysconfdir}@ \
	    <$^ >$@

#
# pluggable authentication modules support
#

if USE_PAM

pamdir   = $(sysconfdir)/pam.d

install-data-local: netatalk
	$(INSTALL_DATA) netatalk $(DESTDIR)$(pamdir)/

endif 
