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

# how to do symbolic links to psf???
# LINKS = ofpap ifpap tfpap ifpaprev tfpaprev ofwpap ifwpap tfwpap ifwpaprev tfwpaprev ofmpap ifmpap tfmpap ifmpaprev tfmpaprev ofwmpap ifwmpap tfwmpap ifwmpaprev tfwmpaprev
