# Makefile.am for libatalk/

# This is the version info for the libatalk binary API.  It has three
# numbers:
#
#   Current  -- the number of the binary API that we're implementing
#   Revision -- which iteration of the implementation of the binary
#               API are we supplying?
#   Age      -- How many previous binary API versions do we also
#               support?
#
# To increment a VERSION_INFO (current:revision:age):
#    If the ABI didn't change, but any library code changed:
#        current:revision+1:age
#    If the ABI changed, but it's backward-compatible:
#        current+1:0:age+1
#    If the ABI changed and it isn't backward-compatible:
#        current+1:0:0
#

VERSION_INFO = 18:0:0

# History:          VERSION_INFO
#
#   3.0.0-alpha1    0:0:0
#   3.0.0-alpha2    0:0:0
#   3.0.0-alpha3    0:0:0
#   3.0.0-beta1     0:0:0
#   3.0.0-beta2     1:0:0
#   3.0             1:0:0
#   3.0.1           2:0:0
#   3.0.2           3:0:0
#   3.0.3           4:0:0
#   3.0.4           5:0:0
#   3.0.5           6:0:0
#   3.0.6           7:0:0

#   3.1.0           12:0:0
#   3.1.1           13:0:0
#   3.1.2           14:0:0
#   3.1.3           15:0:0
#   3.1.4           16:0:0
#   3.1.5           16:0:0
#   3.1.6           16:0:0
#   3.1.7           forgotten
#   3.1.8           17:0:0

# Belatedly following suite with the Meson build system,
# where ABI version is defined to 18 in 3.2.0 and later.
#   3.2.4           18:0:0

SUBDIRS = acl adouble bstring compat cnid dsi iniparser util unicode vfs

lib_LTLIBRARIES = libatalk.la

libatalk_la_SOURCES = dummy.c

libatalk_la_CFLAGS = \
	@PTHREAD_CFLAGS@

libatalk_la_LIBADD  = \
	@WRAP_LIBS@ @ACL_LIBS@ @PTHREAD_LIBS@ @MYSQL_LIBS@ \
	acl/libacl.la \
	adouble/libadouble.la	\
	bstring/libbstring.la \
	cnid/libcnid.la \
	compat/libcompat.la	\
	dsi/libdsi.la		\
	iniparser/libiniparser.la \
	unicode/libunicode.la \
	util/libutil.la		\
	vfs/libvfs.la

libatalk_la_DEPENDENCIES = \
	acl/libacl.la \
	adouble/libadouble.la	\
	bstring/libbstring.la \
	cnid/libcnid.la \
	compat/libcompat.la	\
	dsi/libdsi.la		\
	iniparser/libiniparser.la \
	unicode/libunicode.la \
	util/libutil.la		\
	vfs/libvfs.la

libatalk_la_LDFLAGS = -version-info $(VERSION_INFO)

if WITH_SPOTLIGHT
SUBDIRS += talloc
libatalk_la_LIBADD += talloc/libtalloc.la
libatalk_la_DEPENDENCIES += talloc/libtalloc.la 
endif

