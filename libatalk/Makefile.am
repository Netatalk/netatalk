
# Makefile.am for libatalk/

SUBDIRS = adouble asp atp compat cnid dsi nbp netddp util tdb unicode acl

lib_LTLIBRARIES = libatalk.la

libatalk_la_SOURCES = dummy.c

libatalk_la_LIBADD  = \
	adouble/libadouble.la	\
	asp/libasp.la		\
	atp/libatp.la		\
	compat/libcompat.la	\
	dsi/libdsi.la		\
	nbp/libnbp.la		\
	netddp/libnetddp.la	\
	util/libutil.la		\
        tdb/libtdb.la           \
	unicode/libunicode.la @LIBATALK_ACLS@

libatalk_la_DEPENDENCIES = \
	adouble/libadouble.la	\
	asp/libasp.la		\
	atp/libatp.la		\
	compat/libcompat.la	\
	dsi/libdsi.la		\
	nbp/libnbp.la		\
	netddp/libnetddp.la	\
	util/libutil.la		\
        tdb/libtdb.la           \
	unicode/libunicode.la @LIBATALK_ACLS@

libatalk_la_LDFLAGS = -static

