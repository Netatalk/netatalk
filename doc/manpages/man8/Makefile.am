XSLTPROC=@XSLTPROC@
XSLTPROC_FLAGS=@XSLTPROC_FLAGS@
MAN_STYLESHEET=$(top_srcdir)/doc/man.xsl

MAN_MANPAGES = \
	afpd.8 \
	cnid_dbd.8 \
	cnid_metad.8 \
	netatalk.8

CLEANFILES = $(MAN_MANPAGES)

%.8 : $(MAN_STYLESHEET) %.8.xml
	@xsltproc $(MAN_STYLESHEET) $<
	@cp $@ $(top_builddir)/man/man8/$@.in

html-local: $(MAN_MANPAGES)
