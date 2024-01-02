# Makefile.am for test/afpd/

tmpconfdir = /tmp/netatalk

TESTS = test.sh test

check_PROGRAMS = test
EXTRA_DIST = test.sh test.h subtests.h afpfunc_helpers.h
CLEANFILES = test.default test.conf

test_SOURCES =  test.c subtests.c afpfunc_helpers.c \
				$(top_builddir)/etc/afpd/afp_asp.c \
				$(top_builddir)/etc/afpd/afp_avahi.c \
				$(top_builddir)/etc/afpd/afp_mdns.c \
				$(top_builddir)/etc/afpd/afp_config.c \
				$(top_builddir)/etc/afpd/afp_dsi.c \
				$(top_builddir)/etc/afpd/afp_options.c \
				$(top_builddir)/etc/afpd/afp_util.c \
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
	 @SLP_CFLAGS@ @ZEROCONF_CFLAGS@ @QUOTA_CFLAGS@ \
	 -DAPPLCNAME \
	 -DSERVERTEXT=\"$(SERVERTEXT)/\" \
	 -D_PATH_AFPDDEFVOL=\"$(tmpconfdir)/AppleVolumes.default\" \
	 -D_PATH_AFPDSYSVOL=\"$(tmpconfdir)/AppleVolumes.system\" \
	 -D_PATH_AFPDPWFILE=\"$(tmpconfdir)/afppasswd\" \
	 -D_PATH_AFPDCONF=\"$(tmpconfdir)/afpd.conf\" \
	 -D_PATH_AFPDUAMPATH=\"$(tmpconfdir)/\" \
	 -D_PATH_ACL_LDAPCONF=\"$(tmpconfdir)/afp_ldap.conf\" \
	 -D_PATH_AFPDSIGCONF=\"$(tmpconfdir)/afp_signature.conf\" \
	 -D_PATH_AFPDUUIDCONF=\"$(tmpconfdir)/afp_voluuid.conf\"

test_LDADD = $(top_builddir)/libatalk/cnid/libcnid.la \
	$(top_builddir)/libatalk/libatalk.la \
	@QUOTA_LIBS@ @SLP_LIBS@ @WRAP_LIBS@ @LIBADD_DL@ @ACL_LIBS@ @ZEROCONF_LIBS@ @PTHREAD_LIBS@

test_LDFLAGS = -export-dynamic
