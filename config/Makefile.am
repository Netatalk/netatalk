## Makefile.am for config/

SUFFIXES = .tmpl .

.tmpl:
	sed -e s@:UAMS_PATH:@${UAMS_PATH}@ \
	    -e s@:NLS_PATH:@${CONFIG_DIR}/nls@ \
	    -e s@:ETCDIR:@${CONFIG_DIR}@ \
	    <$^ >$@

CLEANFILES = afpd.conf netatalk

configdir = $(CONFIG_DIR)

config_DATA = AppleVolumes.default AppleVolumes.system afpd.conf atalkd.conf netatalk.conf netatalk.pamd papd.conf

EXTRA_DIST = $(config_DATA)

#
# pluggable authentication modules support?
#

if USE_PAM

pamdir    = /etc/pam.d

pam_DATA = netatalk

netatalk:
	cp -f netatalk.pamd netatalk

endif 
