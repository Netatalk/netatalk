# Makefile.am for test/afpd/

pkgconfdir = @PKGCONFDIR@

TESTS = test.sh test

check_PROGRAMS = test
noinst_HEADERS = test.h subtests.h afpfunc_helpers.h
EXTRA_DIST = test.sh
CLEANFILES = test.default test.conf

test_SOURCES =  test.c subtests.c afpfunc_helpers.c \
				$(top_builddir)/etc/afpd/afp_avahi.c \
				$(top_builddir)/etc/afpd/afp_config.c \
				$(top_builddir)/etc/afpd/afp_dsi.c \
				$(top_builddir)/etc/afpd/afp_options.c \
				$(top_builddir)/etc/afpd/afp_util.c \
				$(top_builddir)/etc/afpd/afprun.c \
				$(top_builddir)/etc/afpd/appl.c \
				$(top_builddir)/etc/afpd/auth.c \
				$(top_builddir)/etc/afpd/afp_zeroconf.c \
				$(top_builddir)/etc/afpd/catsearch.c \
				$(top_builddir)/etc/afpd/desktop.c \
				$(top_builddir)/etc/afpd/dircache.c \
				$(top_builddir)/etc/afpd/directory.c \
				$(top_builddir)/etc/afpd/enumerate.c \
				$(top_builddir)/etc/afpd/extattrs.c \
				$(top_builddir)/etc/afpd/fce_api.c \
				$(top_builddir)/etc/afpd/fce_util.c \
				$(top_builddir)/etc/afpd/file.c \
				$(top_builddir)/etc/afpd/filedir.c \
				$(top_builddir)/etc/afpd/fork.c \
				$(top_builddir)/etc/afpd/gettok.c \
				$(top_builddir)/etc/afpd/hash.c \
				$(top_builddir)/etc/afpd/mangle.c \
				$(top_builddir)/etc/afpd/messages.c \
				$(top_builddir)/etc/afpd/nfsquota.c \
				$(top_builddir)/etc/afpd/ofork.c \
				$(top_builddir)/etc/afpd/quota.c \
				$(top_builddir)/etc/afpd/status.c \
				$(top_builddir)/etc/afpd/switch.c \
				$(top_builddir)/etc/afpd/uam.c \
				$(top_builddir)/etc/afpd/unix.c \
				$(top_builddir)/etc/afpd/volume.c

if HAVE_ACLS
test_SOURCES += $(top_builddir)/etc/afpd/acls.c
endif

test_CFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/etc/afpd \
	 @SLP_CFLAGS@ @ZEROCONF_CFLAGS@ \
	 -DAPPLCNAME \
	 -DSERVERTEXT=\"$(SERVERTEXT)/\" \
	 -D_PATH_AFPDDEFVOL=\"$(pkgconfdir)/AppleVolumes.default\" \
	 -D_PATH_AFPDSYSVOL=\"$(pkgconfdir)/AppleVolumes.system\" \
	 -D_PATH_AFPDPWFILE=\"$(pkgconfdir)/afppasswd\" \
	 -D_PATH_AFPDCONF=\"$(pkgconfdir)/afpd.conf\" \
	 -D_PATH_AFPDUAMPATH=\"$(UAMS_PATH)/\" \
	 -D_PATH_ACL_LDAPCONF=\"$(pkgconfdir)/afp_ldap.conf\" \
	 -D_PATH_AFPDSIGCONF=\"$(pkgconfdir)/afp_signature.conf\" \
	 -D_PATH_AFPDUUIDCONF=\"$(pkgconfdir)/afp_voluuid.conf\"

test_LDADD = $(top_builddir)/libatalk/cnid/libcnid.la \
	$(top_builddir)/libatalk/libatalk.la \
	@QUOTA_LIBS@ @SLP_LIBS@ @WRAP_LIBS@ @LIBADD_DL@ @ACL_LIBS@ @ZEROCONF_LIBS@ @PTHREAD_LIBS@

test_LDFLAGS = -export-dynamic
