# Makefile.am for etc/afpd/

SUBDIRS = nls

sbin_PROGRAMS = afpd

afpd_SOURCES = unix.c ofork.c main.c switch.c auth.c volume.c directory.c file.c enumerate.c desktop.c filedir.c fork.c appl.c gettok.c status.c afp_options.c afp_asp.c afp_dsi.c messages.c config.c nfsquota.c codepage.c quota.c uam.c afs.c
afpd_LDADD = ../../libatalk/libatalk.a -latalk

noinst_HEADERS = auth.h codepage.h config.h desktop.h directory.h file.h filedir.h fork.h globals.h icon.h misc.h status.h switch.h uam_auth.h unix.h volume.h

CFLAGS = @CFLAGS@ -I../../include -I../../sys \
	 -D_PATH_AFPDDEFVOL=\"$(CONFIG_DIR)/AppleVolumes.default\" \
	 -D_PATH_AFPDSYSVOL=\"$(CONFIG_DIR)/AppleVolumes.system\" \
	 -D_PATH_AFPDPWFILE=\"$(CONFIG_DIR)/afpd.passwd\" \
	 -D_PATH_AFPDCONF=\"$(CONFIG_DIR)/afpd.conf\" \
	 -D_PATH_AFPDUAMPATH=\"$(UAMS_PATH)/\" \
	 -D_PATH_AFPDNLSPATH=\"$(CONFIG_DIR)/nls/\" \
	 -DAPPLCNAME \
	 -DVERSION=\"$(NETATALK_VERSION)\"
