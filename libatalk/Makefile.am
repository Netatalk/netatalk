# Makefile.am for libatalk/

# FIXME: Included manually, because for some strange reason automake doesn't.
# (srittau)
CCLD = $(CC)
LINK = $(LIBTOOL) --mode=link $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(LDFLAGS) -o $@

SUBDIRS = adouble asp atp compat dsi nbp netddp util

lib_LTLIBRARIES = libatalk.la

LIBATALK_DEPS = asp/libasp.la atp/libatp.la nbp/libnbp.la adouble/libadouble.la dsi/libdsi.la util/libutil.la compat/libcompat.la netddp/libnetddp.la asp/libasp.la

libatalk_la_SOURCES =
libatalk_la_LIBADD  = ${LIBATALK_DEPS}
libatalk_la_LDFLAGS = -static

#libatalk.a: ${LIBATALK_DEPS}
#	${LIBTOOL} --mode=link ${LIBATALK_DEPS} -o libatalk.a
