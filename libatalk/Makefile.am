
# Makefile.am for libatalk/

SUBDIRS = adouble asp atp bstring compat cnid dsi nbp netddp tdb util unicode vfs

lib_LTLIBRARIES = libatalk.la

libatalk_la_SOURCES = dummy.c

libatalk_la_LIBADD  = \
	adouble/libadouble.la	\
	asp/libasp.la		\
	atp/libatp.la		\
	bstring/libbstring.la \
	compat/libcompat.la	\
	dsi/libdsi.la		\
	nbp/libnbp.la		\
	netddp/libnetddp.la	\
	util/libutil.la		\
	tdb/libtdb.la       \
	unicode/libunicode.la \
	vfs/libvfs.la

libatalk_la_DEPENDENCIES = \
	adouble/libadouble.la	\
	asp/libasp.la		\
	atp/libatp.la		\
	bstring/libbstring.la \
	compat/libcompat.la	\
	dsi/libdsi.la		\
	nbp/libnbp.la		\
	netddp/libnetddp.la	\
	util/libutil.la		\
	tdb/libtdb.la       \
	unicode/libunicode.la \
	vfs/libvfs.la

if HAVE_ACLS
SUBDIRS += acl
libatalk_la_DEPENDENCIES += acl/libacl.la
libatalk_la_LIBADD += acl/libacl.la
endif

libatalk_la_LDFLAGS = -static

