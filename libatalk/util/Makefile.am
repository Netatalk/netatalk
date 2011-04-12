# Makefile.am for libatalk/util/

noinst_LTLIBRARIES = libutil.la

AM_CFLAGS = -I$(top_srcdir)/sys

libutil_la_SOURCES = \
	atalk_addr.c	\
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
	strcasestr.c    \
	strdicasecmp.c	\
	strlcpy.c	\
	volinfo.c \
	unix.c

if HAVE_ATFUNCS
libutil_la_SOURCES += ftw.c
endif
