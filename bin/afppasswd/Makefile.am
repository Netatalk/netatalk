# Makefile.am for bin/afppasswd/

pkgconfdir = @PKGCONFDIR@

if USE_DHX
bin_PROGRAMS = afppasswd
else
bin_PROGRAMS =
endif

afppasswd_SOURCES = afppasswd.c
afppasswd_LDADD = $(top_builddir)/libatalk/libatalk.la @SSL_LIBS@

CFLAGS = @SSL_CFLAGS@ -D_PATH_AFPDPWFILE=\"$(pkgconfdir)/afppasswd\"
