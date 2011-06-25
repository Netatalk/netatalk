# Makefile.am for etc/afpd/

pkgconfdir = @PKGCONFDIR@

sbin_PROGRAMS = afpd
noinst_PROGRAMS = hash fce

afpd_SOURCES = \
	afp_asp.c \
	afp_avahi.c \
	afp_config.c \
	afp_dsi.c \
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
	gettok.c \
	hash.c \
	main.c \
	mangle.c \
	messages.c  \
	nfsquota.c \
	ofork.c \
	quota.c \
	status.c \
	switch.c \
	uam.c \
	uid.c \
	unix.c \
	volume.c

afpd_LDADD =  \
	$(top_builddir)/libatalk/cnid/libcnid.la \
	$(top_builddir)/libatalk/libatalk.la \
	@LIBGCRYPT_LIBS@ @ZEROCONF_LIBS@ @QUOTA_LIBS@ @SLP_LIBS@ @WRAP_LIBS@ @LIBADD_DL@ @ACL_LIBS@ @PTHREAD_LIBS@

afpd_LDFLAGS = -export-dynamic 

afpd_CFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/sys \
	@SLP_CFLAGS@ @ZEROCONF_CFLAGS@ \
	-DAPPLCNAME \
	-DSERVERTEXT=\"$(SERVERTEXT)/\" \
	-D_PATH_AFPDDEFVOL=\"$(pkgconfdir)/AppleVolumes.default\" \
	-D_PATH_AFPDSYSVOL=\"$(pkgconfdir)/AppleVolumes.system\" \
	-D_PATH_AFPDPWFILE=\"$(pkgconfdir)/afppasswd\" \
	-D_PATH_AFPDCONF=\"$(pkgconfdir)/afpd.conf\" \
	-D_PATH_AFPDSIGCONF=\"$(pkgconfdir)/afp_signature.conf\" \
	-D_PATH_AFPDUAMPATH=\"$(UAMS_PATH)/\" \
	-D_PATH_ACL_LDAPCONF=\"$(pkgconfdir)/afp_ldap.conf\" \
	-D_PATH_AFPDUUIDCONF=\"$(pkgconfdir)/afp_voluuid.conf\"

if HAVE_ACLS
afpd_SOURCES += acls.c
endif


noinst_HEADERS = auth.h afp_config.h desktop.h directory.h fce_api_internal.h file.h \
	 filedir.h fork.h icon.h mangle.h misc.h status.h switch.h \
	 uam_auth.h uid.h unix.h volume.h hash.h acls.h acl_mappings.h extattrs.h \
	 dircache.h afp_zeroconf.h afp_avahi.h

hash_SOURCES = hash.c
hash_CFLAGS = -DKAZLIB_TEST_MAIN -I$(top_srcdir)/include

fce_SOURCES = fce_api.c fce_util.c
fce_CFLAGS = -DFCE_TEST_MAIN -I$(top_srcdir)/include
fce_LDADD = $(top_builddir)/libatalk/libatalk.la
