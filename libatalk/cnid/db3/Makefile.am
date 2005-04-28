# Makefile.am for libatalk/cnid/

CFLAGS = @CFLAGS@ @BDB_CFLAGS@

noinst_LTLIBRARIES = libcnid_db3.la
if USE_CDB_BACKEND
LIBS_db = @BDB_LIBS@
else
LIBS_db = 
endif

libcnid_db3_la_SOURCES = cnid_db3_add.c \
		         cnid_db3_close.c \
			 cnid_db3_delete.c \
			 cnid_db3_get.c \
			 cnid_db3_lookup.c \
			 cnid_db3_open.c \
			 cnid_db3_resolve.c \
			 cnid_db3_update.c \
			 cnid_db3.h

noinst_HEADERS = cnid_db3_meta.h cnid_db3_private.h 

libcnid_db3_la_LIBADD = $(LIBS_db)

EXTRA_DIST = README cnid_db3_meta.c cnid_db3_nextid.c
