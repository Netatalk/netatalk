
# Makefile.am for libatalk/

SUBDIRS = acl adouble bstring compat cnid dsi tdb util unicode vfs

lib_LTLIBRARIES = libatalk.la

libatalk_la_SOURCES = dummy.c

libatalk_la_LIBADD  = \
	acl/libacl.la \
	adouble/libadouble.la	\
	bstring/libbstring.la \
	compat/libcompat.la	\
	dsi/libdsi.la		\
	util/libutil.la		\
	tdb/libtdb.la       \
	unicode/libunicode.la \
	vfs/libvfs.la

libatalk_la_DEPENDENCIES = \
	acl/libacl.la \
	adouble/libadouble.la	\
	bstring/libbstring.la \
	compat/libcompat.la	\
	dsi/libdsi.la		\
	util/libutil.la		\
	tdb/libtdb.la       \
	unicode/libunicode.la \
	vfs/libvfs.la

libatalk_la_LDFLAGS = -static

if USE_APPLETALK
SUBDIRS +=  asp atp nbp netddp
libatalk_la_LIBADD += \
	asp/libasp.la		\
	atp/libatp.la		\
	nbp/libnbp.la		\
	netddp/libnetddp.la
libatalk_la_DEPENDENCIES += \
	asp/libasp.la		\
	atp/libatp.la		\
	nbp/libnbp.la		\
	netddp/libnetddp.la
endif
