
# Makefile.am for libatalk/

# rpc should be first because it generates RPC headers included elsewhere
SUBDIRS = rpc acl adouble bstring compat cnid dsi tdb util unicode vfs

lib_LTLIBRARIES = libatalk.la

libatalk_la_SOURCES = dummy.c

libatalk_la_LIBADD  = \
	acl/libacl.la \
	adouble/libadouble.la	\
	bstring/libbstring.la \
	compat/libcompat.la	\
	dsi/libdsi.la		\
	rpc/librpc.la		\
	tdb/libtdb.la       \
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
	tdb/libtdb.la       \
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
