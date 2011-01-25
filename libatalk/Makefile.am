
# Makefile.am for libatalk/

SUBDIRS = acl adouble bstring compat cnid dsi rpc talloc tdb tevent tsocket util unicode vfs

lib_LTLIBRARIES = libatalk.la

libatalk_la_SOURCES = dummy.c

libatalk_la_LIBADD  = \
	acl/libacl.la \
	adouble/libadouble.la	\
	bstring/libbstring.la \
	compat/libcompat.la	\
	dsi/libdsi.la		\
	rpc/librpc.la		\
	talloc/libtalloc.la \
	tdb/libtdb.la       \
	tevent/libtevent.la \
	tsocket/libtsocket.la \
	unicode/libunicode.la \
	util/libutil.la		\
	vfs/libvfs.la		\
	$(top_builddir)/libevent/libevent.la

libatalk_la_DEPENDENCIES = \
	acl/libacl.la \
	adouble/libadouble.la	\
	bstring/libbstring.la \
	compat/libcompat.la	\
	dsi/libdsi.la		\
	rpc/librpc.la		\
	talloc/libtalloc.la \
	tdb/libtdb.la       \
	tevent/libtevent.la \
	tsocket/libtsocket.la \
	unicode/libunicode.la \
	util/libutil.la		\
	vfs/libvfs.la		\
	$(top_builddir)/libevent/libevent.la

libatalk_la_LDFLAGS = -static

if USE_APPLETALK
SUBDIRS += asp atp nbp netddp
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
