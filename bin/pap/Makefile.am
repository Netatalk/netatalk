# Makefile.am for bin/pap/

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/sys

bin_PROGRAMS = pap papstatus

pap_SOURCES = pap.c
pap_LDADD = $(top_builddir)/libatalk/libatalk.la

papstatus_SOURCES = papstatus.c
papstatus_LDADD = $(top_builddir)/libatalk/libatalk.la
