# Makefile.am for bin/cnid/

EXTRA_DIST = cnid_maint.in cnid2_create.in
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/sys

#if COMPILE_CNID
bin_SCRIPTS = cnid_maint cnid2_create
bin_PROGRAMS = cnid_index
#else
#bin_SCRIPTS = 
#endif

cnid_index_SOURCES = cnid_index.c 

cnid_index_LDADD = $(top_builddir)/libatalk/libatalk.la

LIBS = @LIBS@ @BDB_LIBS@
 
CFLAGS = @CFLAGS@ @BDB_CFLAGS@ 
