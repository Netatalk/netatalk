# Makefile.am for libatalk/

SUBDIRS = adouble asp atp compat dsi nbp netddp util

lib_LIBRARIES = libatalk.a

LIBATALK_DEPS = asp/libasp.la atp/libatp.la nbp/libnbp.la adouble/libadouble.la dsi/libdsi.la util/libutil.la compat/libcompat.la netddp/libnetddp.la asp/libasp.la

libatalk_a_SOURCES =
libatalk_a_LIBADD = ${LIBATALK_DEPS}

libatalk.a: ${LIBATALK_DEPS}
	${LIBTOOL} --mode=link ${LIBATALK_DEPS} -o libatalk.a
