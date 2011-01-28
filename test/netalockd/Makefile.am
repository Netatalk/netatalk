# Makefile.am for test/netalockd/

pkgconfdir = @PKGCONFDIR@

TESTS = test.sh
EXTRA_DIST = test.sh
CLEANFILES = test.log
check_PROGRAMS = test

test_SOURCES = test.c
test_LDADD = $(top_builddir)/libatalk/libatalk.la @PTHREAD_LIBS@
test_LDFLAGS = -static
