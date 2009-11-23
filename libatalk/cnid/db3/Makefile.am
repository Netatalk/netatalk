# Makefile.am for libatalk/cnid/

libcnid_db3_la_SOURCES = cnid_db3_add.c \
		     cnid_db3_close.c \
			 cnid_db3_delete.c \
			 cnid_db3_get.c \
			 cnid_db3_lookup.c \
			 cnid_db3_open.c \
			 cnid_db3_resolve.c \
			 cnid_db3_update.c \
			 cnid_db3.h
libcnid_db3_la_LIBADD = @BDB_LIBS@
libcnid_db3_la_CFLAGS = @BDB_CFLAGS@

if USE_DB3_BACKEND
noinst_LTLIBRARIES = libcnid_db3.la
endif

noinst_HEADERS = cnid_db3_meta.h cnid_db3_private.h 
EXTRA_DIST = README cnid_db3_meta.c cnid_db3_nextid.c
