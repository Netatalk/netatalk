# Makefile.am for etc/psf/

sbin_PROGRAMS = psf psa

pkgdata_DATA = pagecount.ps

psf_SOURCES = psf.c
psa_SOURCES = psa.c

CFLAGS = @CFLAGS@ -DZEROWIDTH \
	-D_PATH_PAP=\"$(bindir)/pap\" \
	-D_PATH_PSORDER=\"$(bindir)/psorder\" \
	-D_PATH_PSA=\"$(sbindir)/psa\" \
	-D_PATH_PSFILTER=\"$(bindir)/etc2ps.sh\" \
	-D_PATH_PAGECOUNT=\"$(pkgdatadir)/pagecount.ps\"

EXTRA_DIST = etc2ps.sh pagecount.ps

# declare links
psf_LINKS = ofpap ifpap tfpap ifpaprev tfpaprev ofwpap ifwpap \
	 tfwpap ifwpaprev tfwpaprev ofmpap ifmpap tfmpap ifmpaprev \
	 tfmpaprev ofwmpap ifwmpap tfwmpap ifwmpaprev tfwmpaprev


#
# install sections for links
#

install-exec-local:
	@list='$(psf_LINKS)'; for l in $$list; do \
		rm $(DESTDIR)$(sbindir)/$$l || true; \
		$(LN_S) psf $(DESTDIR)$(sbindir)/$$l;  \
	done

#
# cleanup
#

CLEANFILES = $(psf_LINKS)
