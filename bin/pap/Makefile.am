# Makefile.am for bin/pap/

bin_PROGRAMS = pap papstatus

pap_SOURCES = pap.c
pap_LDADD = $(top_srcdir)/libatalk/libatalk.a

papstatus_SOURCES = papstatus.c
papstatus_LDADD = $(top_srcdir)/libatalk/libatalk.a

CFLAGS = @CFLAGS@ -I$(top_srcdir)/include -I$(top_srcdir)/sys
