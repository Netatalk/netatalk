# Makefile.am for bin/cnid/

CFLAGS = @CFLAGS@ @DB3_CFLAGS@
LIBS = @LIBS@ @DB3_LIBS@

if COMPILE_CNID
bin_PROGRAMS = cnid_didname_verify
else
bin_PROGRAMS =
endif

cnid_didname_verify_SOURCES = cnid_didname_verify.c
