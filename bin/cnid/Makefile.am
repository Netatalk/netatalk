# Makefile.am for bin/cnid/

EXTRA_DIST = cnid_maint.in cnid2_create.in
noinst_HEADERS = ad.h

if USE_BDB
bin_PROGRAMS = ad
bin_SCRIPTS = cnid_maint cnid2_create

ad_SOURCES = ad.c ad_util.c \
	ad_ls.c \
	ad_cp.c

ad_LDADD = $(top_builddir)/libatalk/libatalk.la
endif
