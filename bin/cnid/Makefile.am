# Makefile.am for bin/cnid/

EXTRA_DIST = cnid_maint.in

CFLAGS = @CFLAGS@ @DB3_CFLAGS@
LIBS = @LIBS@ @DB3_LIBS@

if COMPILE_CNID
bin_PROGRAMS = cnid_didname_verify
bin_SCRIPTS = cnid_maint
else
bin_PROGRAMS =
bin_SCRIPTS = 
endif

cnid_didname_verify_SOURCES = cnid_didname_verify.c
