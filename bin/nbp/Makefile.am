# Makefile.am for bin/nbp/

bin_PROGRAMS = nbplkup nbprgstr nbpunrgstr

nbplkup_SOURCES = nbplkup.c
nbplkup_LDADD = $(top_srcdir)/libatalk/libatalk.a

nbprgstr_SOURCES = nbprgstr.c
nbprgstr_LDADD = $(top_srcdir)/libatalk/libatalk.a

nbpunrgstr_SOURCES = nbpunrgstr.c
nbpunrgstr_LDADD = $(top_srcdir)/libatalk/libatalk.a

CFLAGS = @CFLAGS@ -I$(top_srcdir)/include -I$(top_srcdir)/sys
