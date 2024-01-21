XSLTPROC=@XSLTPROC@
XSLTPROC_FLAGS=@XSLTPROC_FLAGS@
MAN_STYLESHEET=$(top_srcdir)/doc/man.xsl
CLEANFILES =

MAN_MANPAGES = \
	atalk_aton.3 \
	nbp_name.3

EXTRA_DIST = \
	atalk_aton.3.xml \
	nbp_name.3.xml

if HAVE_XSLTPROC
CLEANFILES += $(MAN_MANPAGES)

$(MAN_MANPAGES) : %.3 : %.3.xml
	@xsltproc $(MAN_STYLESHEET) $<
	@cp $@ $(top_builddir)/man/man3/$@.in

html-local: $(MAN_MANPAGES) $(MAN_STYLESHEET)
endif
