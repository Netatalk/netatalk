# Makefile.am for etc/psf/

libexec_PROGRAMS = psf psa
libexec_SCRIPTS = etc2ps.sh

pkgdata_DATA = pagecount.ps

psf_SOURCES = psf.c
psa_SOURCES = psa.c

AM_CFLAGS = -DZEROWIDTH \
	-D_PATH_PAP=\"$(bindir)/pap\" \
	-D_PATH_PSORDER=\"$(bindir)/psorder\" \
	-D_PATH_PSA=\"$(libexecdir)/psa\" \
	-D_PATH_PSFILTER=\"$(libexecdir)/etc2ps.sh\" \
	-D_PATH_PAGECOUNT=\"$(pkgdatadir)/pagecount.ps\"

EXTRA_DIST = $(libexec_SCRIPTS) $(pkgdata_DATA)

# declare links
psf_LINKS = ofpap ifpap tfpap ifpaprev tfpaprev ofwpap ifwpap \
	 tfwpap ifwpaprev tfwpaprev ofmpap ifmpap tfmpap ifmpaprev \
	 tfmpaprev ofwmpap ifwmpap tfwmpap ifwmpaprev tfwmpaprev


#
# install sections for links
#

install-exec-hook:
	@list='$(psf_LINKS)'; for l in $$list; do \
		rm $(DESTDIR)$(libexecdir)/$$l 2>/dev/null || true; \
		(cd $(DESTDIR)$(libexecdir) && $(LN_S) psf $$l);  \
	done

#
# uninstall sections for links
#

uninstall-hook:
	@list='$(psf_LINKS)'; for l in $$list; do \
		rm -f $(DESTDIR)$(libexecdir)/$$l; \
	done

#
# cleanup
#

CLEANFILES = $(psf_LINKS)
