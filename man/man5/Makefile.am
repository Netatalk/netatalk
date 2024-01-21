# Makefile.am for man/man5/

GENERATED_MANS	=	AppleVolumes.default.5 \
			afpd.conf.5 \
			netatalk.conf.5 \
			afp_ldap.conf.5 \
			afp_signature.conf.5 \
			afp_voluuid.conf.5

STATIC_MANS	=	AppleVolumes.5 \
			AppleVolumes.system.5

if USE_APPLETALK
GENERATED_MANS	+=	atalkd.conf.5 \
			papd.conf.5
endif

man_MANS = $(GENERATED_MANS) $(STATIC_MANS)
DISTCLEANFILES = $(GENERATED_MANS)
EXTRA_DIST = $(STATIC_MANS)
