# Makefile.am for libatalk/cnid/

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/sys @BDB_CFLAGS@

noinst_LTLIBRARIES = libcnid.la

libcnid_la_SOURCES = cnid_add.c cnid_close.c cnid_delete.c cnid_get.c cnid_lookup.c cnid_mangle_add.c cnid_mangle_get.c cnid_open.c cnid_resolve.c cnid_update.c
libcnid_la_LIBADD = @BDB_LIBS@

noinst_HEADERS = cnid_meta.h cnid_private.h 

EXTRA_DIST = README cnid_meta.c cnid_nextid.c
