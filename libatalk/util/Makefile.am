# Makefile.am for libatalk/util/

pkgconfdir = @PKGCONFDIR@

noinst_LTLIBRARIES = libutil.la

libutil_la_SOURCES = \
	atalk_addr.c	\
	bprint.c	\
	fault.c		\
	ftw.c		\
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