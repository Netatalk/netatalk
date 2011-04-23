# Makefile.am for libatalk/util/

pkgconfdir = @PKGCONFDIR@

noinst_LTLIBRARIES = libutil.la

libutil_la_SOURCES = \
	bprint.c	\
	cnid.c		\
	fault.c		\
	getiface.c	\
	locking.c   \
	logger.c	\
	module.c	\
	queue.c     \
	server_child.c	\
	server_ipc.c	\
	server_lock.c	\
	socket.c        \
	strdicasecmp.c	\
	volinfo.c 	\
	unix.c

libutil_la_CFLAGS = -D_PATH_AFPDCONF=\"$(pkgconfdir)/afpd.conf\"

if HAVE_ATFUNCS
libutil_la_SOURCES += ftw.c
endif
