# Makefile.am for etc/afpd/

pkgconfdir = @PKGCONFDIR@

sbin_PROGRAMS = afpd

afpd_SOURCES = unix.c ofork.c main.c switch.c auth.c volume.c directory.c \
	 file.c enumerate.c desktop.c filedir.c fork.c appl.c gettok.c \
	 mangle.c status.c afp_options.c afp_asp.c afp_dsi.c messages.c  \
	 afp_config.c nfsquota.c quota.c uam.c afs.c uid.c afp_util.c \
         catsearch.c afprun.c vfs_adouble.c

afpd_LDADD =  $(top_builddir)/libatalk/cnid/libcnid.la $(top_builddir)/libatalk/libatalk.la
afpd_LDFLAGS = -export-dynamic

noinst_HEADERS = auth.h afp_config.h desktop.h directory.h file.h \
	 filedir.h fork.h globals.h icon.h mangle.h misc.h status.h switch.h \
	 uam_auth.h uid.h unix.h volume.h afp_vfs.h

LIBS = @LIBS@ @PAM_LIBS@ @QUOTA_LIBS@ @SLP_LIBS@ @WRAP_LIBS@ @LIBADD_DL@

CFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/sys \
	 @CFLAGS@ @SLP_CFLAGS@ \
	 -D_PATH_AFPDDEFVOL=\"$(pkgconfdir)/AppleVolumes.default\" \
	 -D_PATH_AFPDSYSVOL=\"$(pkgconfdir)/AppleVolumes.system\" \
	 -D_PATH_AFPDPWFILE=\"$(pkgconfdir)/afppasswd\" \
	 -D_PATH_AFPDCONF=\"$(pkgconfdir)/afpd.conf\" \
	 -D_PATH_AFPDUAMPATH=\"$(UAMS_PATH)/\" \
	 -DAPPLCNAME \
	 -DSERVERTEXT=\"$(SERVERTEXT)/\"
