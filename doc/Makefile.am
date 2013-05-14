SUBDIRS =
XSLTPROC=@XSLTPROC@
XSLTPROC_FLAGS=@XSLTPROC_FLAGS@
XHTML_STYLESHEET=$(top_srcdir)/doc/html.xsl

htmldir = $(prefix)/share/doc/@PACKAGE@
dist_html_DATA = @PACKAGE@.html

if HAVE_XSLTPROC
	SUBDIRS += manpages
endif

distclean-local:
	$(RM) -f $(top_builddir)/doc/html.xsl $(top_builddir)/doc/html.xsl $(top_builddir)/doc/manual/manual.xml
