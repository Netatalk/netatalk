# Makefile.am for bin/misc

pkgconfdir = @PKGCONFDIR@
bin_PROGRAMS =

noinst_PROGRAMS = netacnv logger_test

netacnv_SOURCES = netacnv.c
netacnv_LDADD = $(top_builddir)/libatalk/libatalk.la

logger_test_SOURCES = logger_test.c
logger_test_LDADD = $(top_builddir)/libatalk/libatalk.la

bin_PROGRAMS += afpldaptest
afpldaptest_SOURCES = uuidtest.c
afpldaptest_CFLAGS = -D_PATH_ACL_LDAPCONF=\"$(pkgconfdir)/afp_ldap.conf\"
afpldaptest_LDADD =  $(top_builddir)/libatalk/libatalk.la

