XSLTPROC=@XSLTPROC@
XSLTPROC_FLAGS=@XSLTPROC_FLAGS@
MAN_STYLESHEET=$(top_srcdir)/doc/man.xsl

MAN_MANPAGES = \
	ad.1 \
	afpldaptest.1 \
	afppasswd.1 \
	afpstats.1 \
	apple_dump.1 \
	asip-status.pl.1 \
	dbd.1 \
	macusers.1 \
	megatron.1 \
	netatalk-config.1 \
	uniconv.1

CLEANFILES = $(MAN_MANPAGES)

%.1 : $(MAN_STYLESHEET) %.1.xml
	@xsltproc $(MAN_STYLESHEET) $<
	@cp $@ $(top_builddir)/man/man1/$@.in

html-local: $(MAN_MANPAGES)
