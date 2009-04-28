# Makefile.am for etc/cnid_dbd/

if BUILD_DBD_DAEMON
sbin_PROGRAMS = cnid_dbd cnid_metad
bin_PROGRAMS = dbd
else
sbin_PROGRAMS =
endif

cnid_dbd_SOURCES = dbif.c pack.c comm.c db_param.c main.c \
                   dbd_add.c dbd_get.c dbd_resolve.c dbd_lookup.c \
                   dbd_update.c dbd_delete.c dbd_getstamp.c \
                   dbd_rebuild_add.c dbd_dbcheck.c
cnid_dbd_LDADD = $(top_builddir)/libatalk/libatalk.la @BDB_LIBS@

cnid_metad_SOURCES = cnid_metad.c usockfd.c db_param.c
cnid_metad_LDADD = $(top_builddir)/libatalk/libatalk.la

dbd_SOURCES = dbd.c dbif.c pack.c
dbd_CFLAGS = $(AM_CFLAGS)
dbd_LDADD = $(top_builddir)/libatalk/libatalk.la @BDB_LIBS@

noinst_HEADERS = dbif.h pack.h db_param.h dbd.h usockfd.h comm.h

EXTRA_DIST = README

AM_CFLAGS = @BDB_CFLAGS@ -D_PATH_CNID_DBD=\"$(sbindir)/cnid_dbd\"
