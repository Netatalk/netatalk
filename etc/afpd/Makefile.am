# Makefile.am for etc/afpd/

SUBDIRS = nls

sbin_PROGRAMS = afpd

bin_PROGRAMS = test_parse_mtab

afpd_SOURCES = unix.c ofork.c main.c switch.c auth.c volume.c directory.c \
	 file.c enumerate.c desktop.c filedir.c fork.c appl.c gettok.c \
	 status.c afp_options.c afp_asp.c afp_dsi.c messages.c afp_config.c \
	 nfsquota.c codepage.c quota.c uam.c afs.c uid.c parse_mtab.c

test_parse_mtab_SOURCES = test_parse_mtab.c parse_mtab.c
test_parse_mtab_LDADD = $(top_builddir)/libatalk/libatalk.la

afpd_LDADD = $(top_builddir)/libatalk/libatalk.la
afpd_LDFLAGS = -rdynamic

noinst_HEADERS = auth.h codepage.h afp_config.h desktop.h directory.h file.h \
	 filedir.h fork.h globals.h icon.h misc.h status.h switch.h uam_auth.h \
	 uid.h unix.h volume.h parse_mtab.h

LIBS = @LIBS@ @AFPD_LIBS@

CFLAGS = @CFLAGS@ \
	 -D_PATH_AFPDDEFVOL=\"$(CONFIG_DIR)/AppleVolumes.default\" \
	 -D_PATH_AFPDSYSVOL=\"$(CONFIG_DIR)/AppleVolumes.system\" \
	 -D_PATH_AFPDPWFILE=\"$(CONFIG_DIR)/afppasswd\" \
	 -D_PATH_AFPDCONF=\"$(CONFIG_DIR)/afpd.conf\" \
	 -D_PATH_AFPDUAMPATH=\"$(UAMS_PATH)/\" \
	 -D_PATH_AFPDNLSPATH=\"$(CONFIG_DIR)/nls/\" \
	 -DAPPLCNAME
