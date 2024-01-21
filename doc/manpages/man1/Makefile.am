XSLTPROC=@XSLTPROC@
XSLTPROC_FLAGS=@XSLTPROC_FLAGS@
MAN_STYLESHEET=$(top_srcdir)/doc/man.xsl
CLEANFILES =

MAN_MANPAGES = \
	ad.1 \
	aecho.1 \
	afpldaptest.1 \
	afppasswd.1 \
	apple_dump.1 \
	asip-status.1 \
	cnid2_create.1 \
	dbd.1 \
	getzones.1 \
	macusers.1 \
	megatron.1 \
	nbp.1 \
	netatalk-config.1 \
	pap.1 \
	psorder.1 \
	showppd.1 \
	uniconv.1

EXTRA_DIST = \
	ad.1.xml \
	aecho.1.xml \
	afpldaptest.1.xml \
	afppasswd.1.xml \
	apple_dump.1.xml \
	asip-status.1.xml \
	cnid2_create.1.xml \
	dbd.1.xml \
	getzones.1.xml \
	macusers.1.xml \
	megatron.1.xml \
	nbp.1.xml \
	netatalk-config.1.xml \
	pap.1.xml \
	psorder.1.xml \
	showppd.1.xml \
	uniconv.1.xml

if HAVE_XSLTPROC
CLEANFILES += $(MAN_MANPAGES)

$(MAN_MANPAGES) : %.1 : %.1.xml
	@xsltproc $(MAN_STYLESHEET) $<
	@cp $@ $(top_builddir)/man/man1/$@.in

html-local: $(MAN_MANPAGES) $(MAN_STYLESHEET)
endif
