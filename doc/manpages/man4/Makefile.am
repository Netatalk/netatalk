XSLTPROC=@XSLTPROC@
XSLTPROC_FLAGS=@XSLTPROC_FLAGS@
MAN_STYLESHEET=$(top_srcdir)/doc/man.xsl
CLEANFILES =

MAN_MANPAGES = atalk.4

EXTRA_DIST = atalk.4.xml

if HAVE_XSLTPROC
CLEANFILES += $(MAN_MANPAGES)

$(MAN_MANPAGES) : %.4 : %.4.xml
	@xsltproc $(MAN_STYLESHEET) $<
	@cp $@ $(top_builddir)/man/man4/$@.in

html-local: $(MAN_MANPAGES) $(MAN_STYLESHEET)
endif
