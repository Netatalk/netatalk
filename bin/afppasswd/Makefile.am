# Makefile.am for bin/afppasswd/

bin_PROGRAMS = afppasswd

afppasswd_SOURCES = afppasswd.c
afppasswd_LDADD = $(top_builddir)/libatalk/libatalk.la

CFLAGS = @CFLAGS@ \
	-D_PATH_AFPDPWFILE=\"$(pkgconfdir)/afppasswd\"
