# Makefile.am for libatalk/compat/

noinst_LTLIBRARIES = libcompat.la

EXTRA_DIST = flock.c
libcompat_la_SOURCES =	\
	mktemp.c	\
	getusershell.c	\
	strcasecmp.c	\
	strstr.c	\
if !HAVE_FLOCK
	flock.c		\
endif
	strdup.c	\
	inet_aton.c	\
	rquota_xdr.c 
