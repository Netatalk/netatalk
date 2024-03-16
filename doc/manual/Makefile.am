XSLTPROC=@XSLTPROC@
XSLTPROC_FLAGS=@XSLTPROC_FLAGS@
HTML_STYLESHEET=$(top_srcdir)/doc/html.xsl
CLEANFILES =

XML_PAGES = \
	manual.xml \
	configuration.xml \
	install.xml \
	intro.xml \
	upgrade.xml

EXTRA_DIST = \
	manual.xml.in \
	configuration.xml \
	install.xml \
	intro.xml \
	upgrade.xml \
	netatalk.html.in

HTML_PAGES = \
	ad.1.html \
	afp.conf.5.html \
	afpd.8.html \
	afpldaptest.1.html \
	afppasswd.1.html \
	afp_signature.conf.5.html  \
	afpstats.1.html \
	afp_voluuid.conf.5.html \
	apple_dump.1.html \
	asip-status.1.html \
	cnid_dbd.8.html \
	cnid_metad.8.html \
	configuration.html \
	dbd.1.html \
	example-toc.html \
	extmap.conf.5.html \
	index.html \
	installation.html \
	intro.html \
	macusers.1.html \
	man-pages.html \
	manual-index.html \
	netatalk.8.html \
	netatalkconfig.1.html \
	pr01.html \
	table-toc.html \
	upgrade.html

DISTCLEANFILES = manual.xml natatalk.html

if HAVE_XSLTPROC
CLEANFILES += $(HTML_PAGES)

html-local: $(XML_PAGES)
	@xsltproc $(HTML_STYLESHEET) manual.xml

endif
