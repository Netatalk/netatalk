# Makefile.am for etc/netalockd/

sbin_PROGRAMS = netalockd

netalockd_SOURCES = main.c
netalockd_LDADD = \
	$(top_builddir)/libatalk/libatalk.la \
	$(top_builddir)/libevent/libevent.la \
	@PTHREAD_LIBS@
netalockd_LDFLAGS = -static
