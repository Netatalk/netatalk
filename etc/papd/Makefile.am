# Makefile.am for etc/papd/

pkgconfdir = @PKGCONFDIR@

sbin_PROGRAMS = papd
#bin_PROGRAMS = showppd

papd_SOURCES = main.c printcap.c session.c file.c comment.c lp.c ppd.c \
	magics.c headers.c queries.c auth.c uam.c
papd_LDADD = @PAM_LIBS@ $(top_builddir)/libatalk/libatalk.la
papd_LDFLAGS = -export-dynamic

#showppd_SOURCES = showppd.c ppd.c
#showppd_LDADD = $(top_builddir)/libatalk/libatalk.la

noinst_HEADERS =	\
	comment.h	\
	file.h		\
	lp.h		\
	ppd.h		\
	printcap.h	\
	printer.h	\
	session.h	\
	uam_auth.h

INCLUDES = \
	-I$(top_srcdir)/include -I$(top_srcdir)/sys \
	-D_PATH_PAPDCONF=\"$(pkgconfdir)/papd.conf\" \
	-D_PATH_PAPDUAMPATH=\"$(UAMS_PATH)/\"
