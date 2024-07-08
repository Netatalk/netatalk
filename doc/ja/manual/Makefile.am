XSLTPROC=@XSLTPROC@
XSLTPROC_FLAGS=@XSLTPROC_FLAGS@
HTML_STYLESHEET=$(top_srcdir)/doc/ja/html.xsl
CLEANFILES =

XML_PAGES = \
	configuration.xml \
	gpl.xml \
	install.xml \
	intro.xml \
	manual.xml \
	upgrade.xml

EXTRA_DIST = \
	configuration.xml \
	gpl.xml \
	install.xml \
	intro.xml \
	manual.xml.in \
	pageheader.txt.in \
	upgrade.xml

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
	netatalk-config.1.html \
	pr01.html \
	table-toc.html \
	upgrade.html

DISTCLEANFILES = manual.xml pageheader.txt

if HAVE_XSLTPROC
CLEANFILES += $(HTML_PAGES)

html-local: $(XML_PAGES)
	@xsltproc $(HTML_STYLESHEET) manual.xml

endif
