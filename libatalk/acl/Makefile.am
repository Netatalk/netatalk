# Makefile.am for libatalk/acl/

noinst_HEADERS = aclldap.h cache.h

if HAVE_ACLS
noinst_LTLIBRARIES = libacl.la
libacl_la_SOURCES = \
	ldap.c		\
	uuid.c		\
	cache.c		\
	ldap_config.c \
	unix.c

libacl_la_LDFLAGS = -lldap
endif

