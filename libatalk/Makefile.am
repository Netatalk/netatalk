
# Makefile.am for libatalk/

# rpc should be first because it generates RPC headers included elsewhere
SUBDIRS = acl adouble bstring compat cnid dsi iniparser tdb util unicode vfs

lib_LTLIBRARIES = libatalk.la

libatalk_la_SOURCES = dummy.c

libatalk_la_LIBADD  = \
	@WRAP_LIBS@ @ACL_LIBS@ \
	acl/libacl.la \
	adouble/libadouble.la	\
	bstring/libbstring.la \
	compat/libcompat.la	\
	dsi/libdsi.la		\
	iniparser/libiniparser.la \
	tdb/libtdb.la       \
	unicode/libunicode.la \
	util/libutil.la		\
	vfs/libvfs.la

libatalk_la_DEPENDENCIES = \
	acl/libacl.la \
	adouble/libadouble.la	\
	bstring/libbstring.la \
	compat/libcompat.la	\
	dsi/libdsi.la		\
	iniparser/libiniparser.la \
	tdb/libtdb.la       \
	unicode/libunicode.la \
	util/libutil.la		\
	vfs/libvfs.la

libatalk_la_LDFLAGS = -version-info 0:0:0
