# Makefile.am for libatalk/cnid/

CFLAGS = @CFLAGS@ @DB3_CFLAGS@
LIBS = @LIBS@ @DB3_LIBS@

noinst_LTLIBRARIES = libcnid.la

libcnid_la_SOURCES = cnid_add.c cnid_close.c cnid_delete.c cnid_get.c cnid_lookup.c cnid_open.c cnid_resolve.c cnid_update.c

noinst_HEADERS = cnid_meta.h cnid_private.h 

EXTRA_DIST = README cnid_meta.c cnid_nextid.c
