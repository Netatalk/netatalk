XSLTPROC=@XSLTPROC@
XSLTPROC_FLAGS=@XSLTPROC_FLAGS@
XHTML_STYLESHEET=$(top_srcdir)/doc/html.xsl
MAN_STYLESHEET=$(top_srcdir)/doc/man.xsl

htmldir = $(prefix)/share/doc/@PACKAGE@
dist_html_DATA = @PACKAGE@.html

XML_MANPAGES = \
	${top_srcdir}/doc/manpages/man1/ad.1.xml \
	${top_srcdir}/doc/manpages/man1/afpldaptest.1.xml \
	${top_srcdir}/doc/manpages/man1/afppasswd.1.xml \
	${top_srcdir}/doc/manpages/man1/afpstats.1.xml \
	${top_srcdir}/doc/manpages/man1/apple_dump.1.xml \
	${top_srcdir}/doc/manpages/man1/asip-status.pl.1.xml \
	${top_srcdir}/doc/manpages/man1/dbd.1.xml \
	${top_srcdir}/doc/manpages/man1/macusers.1.xml \
	${top_srcdir}/doc/manpages/man1/megatron.1.xml \
	${top_srcdir}/doc/manpages/man1/netatalk-config.1.xml \
	${top_srcdir}/doc/manpages/man1/uniconv.1.xml \
	${top_srcdir}/doc/manpages/man5/afp_signature.conf.5.xml \
	${top_srcdir}/doc/manpages/man5/afp_voluuid.conf.5.xml \
	${top_srcdir}/doc/manpages/man5/afp.conf.5.xml \
	${top_srcdir}/doc/manpages/man5/extmap.conf.5.xml \
	${top_srcdir}/doc/manpages/man8/afpd.8.xml \
	${top_srcdir}/doc/manpages/man8/cnid_dbd.8.xml \
	${top_srcdir}/doc/manpages/man8/cnid_metad.8.xml \
	${top_srcdir}/doc/manpages/man8/netatalk.8.xml

MAN_MANPAGES = \
	${top_builddir}/man/man1/ad.1.in \
	${top_builddir}/man/man1/afpldaptest.1.in \
	${top_builddir}/man/man1/afppasswd.1.in \
	${top_builddir}/man/man1/afpstats.1.in \
	${top_builddir}/man/man1/apple_dump.1.in \
	${top_builddir}/man/man1/asip-status.pl.1.in \
	${top_builddir}/man/man1/dbd.1.in \
	${top_builddir}/man/man1/macusers.1.in \
	${top_builddir}/man/man1/megatron.1.in \
	${top_builddir}/man/man1/netatalk-config.1.in \
	${top_builddir}/man/man1/uniconv.1.in \
	${top_builddir}/man/man5/afp_signature.conf.5.in \
	${top_builddir}/man/man5/afp_voluuid.conf.5.in \
	${top_builddir}/man/man5/afp.conf.5.in \
	${top_builddir}/man/man5/extmap.conf.5.in \
	${top_builddir}/man/man8/afpd.8.in \
	${top_builddir}/man/man8/cnid_dbd.8.in \
	${top_builddir}/man/man8/cnid_metad.8.in \
	${top_builddir}/man/man8/netatalk.8.in

if HAVE_XSLTPROC
${top_srcdir}/man/man1/%.1.in : $(MAN_STYLESHEET) ${top_srcdir}/doc/manpages/man1/%.1.xml
	@xsltproc -o $(top_builddir)/man/man1/ $(MAN_STYLESHEET) $<
	mv $% $@.in
${top_srcdir}/man/man5/%.8.in : $(MAN_STYLESHEET) ${top_srcdir}/doc/manpages/man5/%.5.xml
	@xsltproc -o $(top_builddir)/man/man5/ $(MAN_STYLESHEET) $<
${top_srcdir}/man/man8/%.8.in : $(MAN_STYLESHEET) ${top_srcdir}/doc/manpages/man8/%.8.xml
	@xsltproc -o $(top_builddir)/man/man8/ $(MAN_STYLESHEET) $<

all-local: $(MAN_MANPAGES)

endif

distclean-local:
	$(RM) -f $(top_builddir)/doc/html.xsl $(top_builddir)/doc/html.xsl $(top_builddir)/doc/manual/manual.xml
