# Makefile.am for libatalk/acl/

noinst_HEADERS = aclldap.h cache.h

noinst_LTLIBRARIES = libacl.la
libacl_la_SOURCES = cache.c unix.c uuid.c
libacl_la_LDFLAGS =

if HAVE_LDAP
libacl_la_SOURCES += ldap.c ldap_config.c
libacl_la_LDFLAGS += -lldap
endif

