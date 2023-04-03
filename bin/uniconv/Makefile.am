# Makefile.am for bin/aecho/

AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/sys

bin_PROGRAMS = uniconv

uniconv_SOURCES = uniconv.c iso8859_1_adapted.c
uniconv_LDADD = $(top_builddir)/libatalk/libatalk.la @MYSQL_LIBS@
