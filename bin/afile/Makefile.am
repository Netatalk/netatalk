# Makefile.am for bin/afile/

bin_PROGRAMS = achfile afile

achfile_SOURCES = achfile.c
achfile_LDADD = $(top_builddir)/libatalk/libatalk.la

afile_SOURCES	= afile.c
afile_LDADD = $(top_builddir)/libatalk/libatalk.la

LIBS = @LIBS@ -L$(top_builddir)/libatalk/

EXTRA_DIST = README acleandir.rc
