pkgconfdir = @PKGCONFDIR@

if USE_NFSv4_ACLS
bin_PROGRAMS = uuidtest

uuidtest_SOURCES = uuidtest.c
uuidtest_LDADD =  $(top_builddir)/libatalk/libatalk.la

AM_CFLAGS = -D_PATH_ACL_LDAPCONF=\"$(pkgconfdir)/ldap.conf\"
endif
