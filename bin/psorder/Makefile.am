# Makefile.am for bin/psorder

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/sys

bin_PROGRAMS = psorder

psorder_SOURCES = psorder.c pa.c
psorder_LDADD = $(top_builddir)/libatalk/libatalk.la

noinst_HEADERS = psorder.h pa.h
