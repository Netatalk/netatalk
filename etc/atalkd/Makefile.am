# Makefile.am for etc/atalkd/

sbin_PROGRAMS = atalkd

atalkd_SOURCES = aep.c config.c main.c multicast.c nbp.c route.c rtmp.c zip.c 
atalkd_LDADD = $(top_builddir)/libatalk/libatalk.la

noinst_HEADERS = atserv.h gate.h interface.h list.h multicast.h nbp.h rtmp.h zip.h

CFLAGS = @CFLAGS@ -D_PATH_ATALKDCONF=\"$(pkgconfdir)/atalkd.conf\"
