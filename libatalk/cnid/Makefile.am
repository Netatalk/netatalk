# Makefile.am for libatalk/cnid/


SUBDIRS = db3 last mtab cdb dbd hash tdb

noinst_LTLIBRARIES = libcnid.la

LIBCNID_DEPS = db3/libcnid_db3.la \
               cdb/libcnid_cdb.la \
               last/libcnid_last.la \
               mtab/libcnid_mtab.la \
               hash/libcnid_hash.la \
               tdb/libcnid_tdb.la \
               dbd/libcnid_dbd.la 

libcnid_la_SOURCES = cnid.c cnid_init.c
libcnid_la_LIBADD = $(LIBCNID_DEPS)

EXTRA_DIST = README

