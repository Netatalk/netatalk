# Makefile.am for etc/afpd/

pkgconfdir = @PKGCONFDIR@

sbin_PROGRAMS = afpd
noinst_PROGRAMS = hash fce spot srp

afpd_SOURCES = \
	afp_avahi.c \
	afp_config.c \
	afp_dsi.c \
	afp_mdns.c \
	afp_options.c \
	afp_util.c \
	afp_zeroconf.c \
	afprun.c \
	afs.c \
	appl.c \
	auth.c \
	catsearch.c \
	desktop.c \
	dircache.c \
	directory.c \
	enumerate.c \
	extattrs.c \
	fce_api.c \
	fce_util.c \
	file.c \
	filedir.c \
	fork.c \
	hash.c \
	main.c \
	mangle.c \
	messages.c  \
	nfsquota.c \
	ofork.c \
	quota.c \
	spotlight.c \
	spotlight_marshalling.c \
	status.c \
	switch.c \
	uam.c \
	uid.c \
	unix.c \
	volume.c

afpd_LDADD =  \
	$(top_builddir)/libatalk/libatalk.la \
	@LIBGCRYPT_LIBS@ @QUOTA_LIBS@ @WRAP_LIBS@ @LIBADD_DL@ @ACL_LIBS@ @ZEROCONF_LIBS@ @PTHREAD_LIBS@ @GSSAPI_LIBS@ @KRB5_LIBS@

afpd_LDFLAGS = -export-dynamic

afpd_CFLAGS = \
	@ZEROCONF_CFLAGS@ @GSSAPI_CFLAGS@ @KRB5_CFLAGS@ @PTHREAD_CFLAGS@\
	-DAPPLCNAME \
	-DSERVERTEXT=\"$(SERVERTEXT)/\" \
	-D_PATH_AFPDPWFILE=\"$(pkgconfdir)/afppasswd\" \
	-D_PATH_AFPDUAMPATH=\"$(UAMS_PATH)/\" \
	-D_PATH_CONFDIR=\"$(pkgconfdir)/\" \
	-D_PATH_STATEDIR='"$(localstatedir)/netatalk/"'

if HAVE_ACLS
afpd_SOURCES += acls.c
endif


noinst_HEADERS = auth.h afp_config.h desktop.h directory.h fce_api_internal.h file.h \
	 filedir.h fork.h icon.h mangle.h misc.h status.h switch.h \
	 uam_auth.h uid.h unix.h volume.h hash.h acls.h acl_mappings.h extattrs.h \
	 dircache.h afp_zeroconf.h afp_avahi.h afp_mdns.h spotlight.h \
	 spotlight_rawquery_parser.h

hash_SOURCES = hash.c
hash_CFLAGS = -DKAZLIB_TEST_MAIN -I$(top_srcdir)/include

fce_SOURCES = fce_api.c fce_util.c
fce_CFLAGS = -DFCE_TEST_MAIN -I$(top_srcdir)/include
fce_LDADD = $(top_builddir)/libatalk/libatalk.la

AM_YFLAGS = -d
BUILT_SOURCES = spotlight_rawquery_parser.h

if HAVE_TRACKER
spot_SOURCES = spotlight.c spotlight_marshalling.c
spot_CFLAGS = -DSPOT_TEST_MAIN
spot_LDADD = $(top_builddir)/libatalk/libatalk.la

moduledir = @UAMS_PATH@
module_LTLIBRARIES = mod_spotlight.la

mod_spotlight_la_SOURCES = \
	spotlight_module.c \
	spotlight_SPARQL_map.c \
	spotlight_rawquery_parser.y \
	spotlight_rawquery_lexer.l
mod_spotlight_la_CFLAGS  = -DDBUS_API_SUBJECT_TO_CHANGE @TRACKER_CFLAGS@ @TRACKER_MINER_CFLAGS@
mod_spotlight_la_LDFLAGS = -module -avoid-version @TRACKER_LIBS@ @TRACKER_MINER_LIBS@

srp_SOURCES = \
	spotlight_SPARQL_map.c \
	spotlight_rawquery_parser.y \
	spotlight_rawquery_lexer.l
srp_CFLAGS = -DMAIN -I$(top_srcdir)/include @TRACKER_CFLAGS@
srp_LDADD = $(top_builddir)/libatalk/libatalk.la
endif
