XSLTPROC=@XSLTPROC@
XSLTPROC_FLAGS=@XSLTPROC_FLAGS@
MAN_STYLESHEET=$(top_srcdir)/doc/man.xsl
CLEANFILES =

MAN_MANPAGES = \
	a2boot.8 \
	afpd.8 \
	atalkd.8 \
	cnid_dbd.8 \
	cnid_metad.8 \
	papd.8 \
	papstatus.8 \
	psf.8 \
	timelord.8

EXTRA_DIST = \
	a2boot.8.xml \
	afpd.8.xml \
	atalkd.8.xml \
	cnid_dbd.8.xml \
	cnid_metad.8.xml \
	papd.8.xml \
	papstatus.8.xml \
	psf.8.xml \
	timelord.8.xml

if HAVE_XSLTPROC
CLEANFILES += $(MAN_MANPAGES)

$(MAN_MANPAGES) : %.8 : %.8.xml
	@xsltproc $(MAN_STYLESHEET) $<
	@cp $@ $(top_builddir)/man/man8/$@.in

html-local: $(MAN_MANPAGES) $(MAN_STYLESHEET)
endif
