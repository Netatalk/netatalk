# Makefile.am for bin/afppasswd/

pkgconfdir = $(PKGCONFDIR)

if USE_DHX
bin_PROGRAMS = afppasswd
else
bin_PROGRAMS =
endif

if HAVE_GCRYPT
CRYPT_CFLAGS = $(GCRYPT_CFLAGS)
CRYPT_LIBS = $(GCRYPT_LIBS)
else
CRYPT_CFLAGS = $(SSL_CFLAGS)
CRYPT_LIBS = $(SSL_LIBS)
endif

afppasswd_SOURCES = afppasswd.c
afppasswd_LDADD = $(top_builddir)/libatalk/libatalk.la $(CRYPT_LIBS)

INCLUDES = $(CRYPT_CFLAGS) -I$(top_srcdir)/sys \
    -D_PATH_AFPDPWFILE=\"$(pkgconfdir)/afppasswd\"

install-exec-hook:
if USE_DHX
	chmod u+s $(DESTDIR)$(bindir)/afppasswd
endif
