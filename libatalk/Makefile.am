# Makefile.am for libatalk/

SUBDIRS = adouble asp atp compat dsi nbp netddp util

lib_LTLIBRARIES = libatalk.la

LIBATALK_DEPS = asp/libasp.la atp/libatp.la nbp/libnbp.la adouble/libadouble.la dsi/libdsi.la util/libutil.la compat/libcompat.la netddp/libnetddp.la asp/libasp.la

libatalk_la_SOURCES = dummy.c
libatalk_la_LIBADD  = ${LIBATALK_DEPS}
libatalk_la_LDFLAGS = -static

