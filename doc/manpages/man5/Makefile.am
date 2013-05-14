XSLTPROC=@XSLTPROC@
XSLTPROC_FLAGS=@XSLTPROC_FLAGS@
MAN_STYLESHEET=$(top_srcdir)/doc/man.xsl

MAN_MANPAGES = \
	afp.conf.5 \
	afp_signature.conf.5 \
	afp_voluuid.conf.5 \
	extmap.conf.5

CLEANFILES = $(MAN_MANPAGES)

%.5 : $(MAN_STYLESHEET) %.5.xml
	@xsltproc $(MAN_STYLESHEET) $<
	@cp $@ $(top_builddir)/man/man5/$@.in

html-local: $(MAN_MANPAGES)
