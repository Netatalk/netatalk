# Makefile.am for bin/misc

bin_PROGRAMS = netacnv

netacnv_SOURCES = netacnv.c
netacnv_LDADD = $(top_builddir)/libatalk/libatalk.la
pkgconfdir = @PKGCONFDIR@

if USE_NFSv4_ACLS
bin_PROGRAMS += afpldaptest

afpldaptest_SOURCES = uuidtest.c
afpldaptest_LDADD =  $(top_builddir)/libatalk/libatalk.la

AM_CFLAGS = -D_PATH_ACL_LDAPCONF=\"$(pkgconfdir)/afp_ldap.conf\"
endif
