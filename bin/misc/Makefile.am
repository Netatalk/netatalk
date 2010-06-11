# Makefile.am for bin/misc

pkgconfdir = @PKGCONFDIR@
bin_PROGRAMS =

noinst_PROGRAMS = netacnv
netacnv_SOURCES = netacnv.c
netacnv_LDADD = $(top_builddir)/libatalk/libatalk.la

if HAVE_ACLS
bin_PROGRAMS += afpldaptest
afpldaptest_SOURCES = uuidtest.c
afpldaptest_CFLAGS = -D_PATH_ACL_LDAPCONF=\"$(pkgconfdir)/afp_ldap.conf\"
afpldaptest_LDADD =  $(top_builddir)/libatalk/libatalk.la
endif
