# Makefile.am for libatalk/compat/

noinst_LTLIBRARIES = libcompat.la

EXTRA_DIST = flock.c

# Do some conditional compiling
if HAVE_FLOCK
COMPILE_FLOCK=
else
COMPILE_FLOCK=flock.c
endif

libcompat_la_SOURCES =	\
	mktemp.c	\
	getusershell.c	\
	strcasecmp.c	\
	strstr.c	\
	strdup.c	\
	inet_aton.c	\
	rquota_xdr.c	\
	$(COMPILE_FLOCK)
