XSLTPROC=@XSLTPROC@
XSLTPROC_FLAGS=@XSLTPROC_FLAGS@
MAN_STYLESHEET=$(top_srcdir)/doc/man.xsl
CLEANFILES =

MAN_MANPAGES = \
	afp_ldap.conf.5 \
	afp_signature.conf.5 \
	afp_voluuid.conf.5 \
	afpd.conf.5 \
	AppleVolumes.default.5 \
	atalkd.conf.5 \
	netatalk.conf.5 \
	papd.conf.5

EXTRA_DIST = \
	afp_ldap.conf.5.xml \
	afp_signature.conf.5.xml \
	afp_voluuid.conf.5.xml \
	afpd.conf.5.xml \
	AppleVolumes.default.5.xml \
	atalkd.conf.5.xml \
	netatalk.conf.5.xml \
	papd.conf.5.xml

if HAVE_XSLTPROC
CLEANFILES += $(MAN_MANPAGES)

$(MAN_MANPAGES) : %.5 : %.5.xml
	@xsltproc $(MAN_STYLESHEET) $<
	@cp $@ $(top_builddir)/man/man5/$@.in

html-local: $(MAN_MANPAGES) $(MAN_STYLESHEET)
endif
