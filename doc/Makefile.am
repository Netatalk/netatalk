SUBDIRS = manpages manual
XSLTPROC=@XSLTPROC@
XSLTPROC_FLAGS=@XSLTPROC_FLAGS@
XHTML_STYLESHEET=$(top_srcdir)/doc/html.xsl

htmldir = $(prefix)/share/doc/@PACKAGE@
dist_html_DATA = @PACKAGE@.html

DISTCLEANFILES = html.xsl man.xsl

release-notes: www/ReleaseNotes
	cd www && ./create-relnotes.sh

upload-release-notes: release-notes
	scp www/ReleaseNotes.html $$USER,netatalk@web.sourceforge.net:/home/project-web/netatalk/htdocs/3.0/ReleaseNotes$(VERSION).html
