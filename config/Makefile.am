## Makefile.am for config/

SUFFIXES = .tmpl .

.tmpl:
	sed -e s@:UAMS_PATH:@${UAMS_PATH}@ \
	    -e s@:NLS_PATH:@${CONFIG_DIR}/nls@ \
	    -e s@:ETCDIR:@${CONFIG_DIR}@ \
	    <$^ >$@

CLEANFILES = afpd.conf

configdir = $(CONFIG_DIR)

config_DATA = AppleVolumes.default AppleVolumes.system afpd.conf atalkd.conf netatalk.conf netatalk.pamd papd.conf
