# Makefile.am for etc/psf/

sbin_PROGRAMS = psf psa

psf_SOURCES = psf.c
psa_SOURCES = psa.c

CFLAGS = @CFLAGS@ -DZEROWIDTH \
	-D_PATH_PAP=\"$(bindir)/pap\" \
	-D_PATH_PSORDER=\"$(bindir)/psorder\" \
	-D_PATH_PSA=\"$(sbindir)/psa\" \
	-D_PATH_PSFILTER=\"$(bindir)/etc2ps.sh\" \
	-D_PATH_PAGECOUNT=\"$(CONFIG_DIR)/pagecount.ps\"

# declare links
psf_LINKS = ofpap ifpap tfpap ifpaprev tfpaprev ofwpap ifwpap \
	 tfwpap ifwpaprev tfwpaprev ofmpap ifmpap tfmpap ifmpaprev \
	 tfmpaprev ofwmpap ifwmpap tfwmpap ifwmpaprev tfwmpaprev


#
# install sections for links
#

install-data-local:
	@list='$(psf_LINKS)'; for l in $$list; do \
		$(LN_S) -f psf $(DESTDIR)$(sbindir)/$$l;  \
	done

#
# cleanup
#

clean distclean clean-recursive distclean-recursive:
	$(RM) $(psf_LINKS) *.o core .libs .deps
