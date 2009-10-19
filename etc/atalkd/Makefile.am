# Makefile.am for etc/atalkd/

pkgconfdir = @PKGCONFDIR@

sbin_PROGRAMS = atalkd

atalkd_SOURCES = aep.c config.c main.c multicast.c nbp.c route.c rtmp.c zip.c 
atalkd_LDADD = $(top_builddir)/libatalk/libatalk.la

noinst_HEADERS =	\
	atserv.h	\
	gate.h		\
	interface.h	\
	list.h		\
	main.h		\
	multicast.h	\
	nbp.h		\
	route.h		\
	rtmp.h		\
	zip.h

AM_CFLAGS = -D_PATH_ATALKDCONF=\"$(pkgconfdir)/atalkd.conf\"
