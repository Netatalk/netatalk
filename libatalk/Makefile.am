# Makefile.am for libatalk/

SUBDIRS = adouble asp atp compat dsi nbp netddp util

lib_LTLIBRARIES = libatalk.la

LIBATALK_DEPS = \
	adouble/libadouble.la	\
	asp/libasp.la		\
	atp/libatp.la		\
	compat/libcompat.la	\
	dsi/libdsi.la		\
	nbp/libnbp.la		\
	netddp/libnetddp.la	\
	util/libutil.la

libatalk_la_SOURCES = dummy.c
libatalk_la_LIBADD  = ${LIBATALK_DEPS}
libatalk_la_LDFLAGS = -static

