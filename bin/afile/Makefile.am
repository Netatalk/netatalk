# Makefile.am for bin/afile/

bin_PROGRAMS = achfile afile

achfile_SOURCES = achfile.c
achfile_LDADD = $(top_srcdir)/libatalk/libatalk.a

afile_SOURCES	= afile.c
afile_LDADD = $(top_srcdir)/libatalk/libatalk.a

LIBS = @LIBS@ -L$(top_srcdir)/libatalk/ -latalk

EXTRA_DIST = README acleandir.rc
