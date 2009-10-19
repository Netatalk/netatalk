# Makefile.am for etc/papd/

pkgconfdir = @PKGCONFDIR@
spooldir = @SPOOLDIR@

sbin_PROGRAMS = papd
bin_PROGRAMS = showppd

papd_SOURCES = main.c printcap.c session.c file.c comment.c lp.c ppd.c \
	       magics.c headers.c queries.c auth.c uam.c print_cups.c

papd_LDADD = $(top_builddir)/libatalk/libatalk.la @CUPS_LIBS@ @LIBADD_DL@
papd_LDFLAGS = -export-dynamic @CUPS_LDFLAGS@

showppd_SOURCES = showppd.c ppd.c
showppd_CFLAGS = -DSHOWPPD
showppd_LDADD = $(top_builddir)/libatalk/libatalk.la

noinst_HEADERS =	\
	comment.h	\
	file.h		\
	lp.h		\
	ppd.h		\
	printcap.h	\
	printer.h	\
	session.h	\
	print_cups.h	\
	uam_auth.h

AM_CFLAGS = \
	-I$(top_srcdir)/include -I$(top_srcdir)/sys \
	@CUPS_CFLAGS@\
	-D_PATH_PAPDCONF=\"$(pkgconfdir)/papd.conf\" \
	-D_PATH_PAPDUAMPATH=\"$(UAMS_PATH)/\" \
	-DSPOOLDIR=\"$(spooldir)/\"

if USE_SPOOLDIR
install-exec-hook:
	echo "Creating SPOOLDIR $(DESTDIR)$(spooldir)..."
	$(mkinstalldirs) $(DESTDIR)$(spooldir)
	chmod 0777 $(DESTDIR)$(spooldir)

else
install-exec-hook:

endif
