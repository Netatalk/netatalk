# Makefile.am for etc/afpd/

SUBDIRS = nls

sbin_PROGRAMS = afpd

afpd_SOURCES = unix.c ofork.c main.c switch.c auth.c volume.c directory.c file.c enumerate.c desktop.c filedir.c fork.c appl.c gettok.c status.c afp_options.c afp_asp.c afp_dsi.c messages.c config.c nfsquota.c codepage.c quota.c uam.c afs.c
afpd_LDADD = afpd_options.o

noinst_HEADERS = auth.h codepage.h config.h desktop.h directory.h file.h filedir.h fork.h globals.h icon.h misc.h status.h switch.h uam_auth.h unix.h volume.h

afpd_options.o: afp_options.c
	${COMPILE} \
	 -D_PATH_AFPDDEFVOL=\"${ETCDIR}/AppleVolumes.default\" \
	 -D_PATH_AFPDSYSVOL=\"${ETCDIR}/AppleVolumes.system\" \
	 -D_PATH_AFPDPWFILE=\"${ETCDIR}/afpd.passwd\" \
	 -D_PATH_AFPDCONF=\"${ETCDIR}/afpd.conf\" \
	 -D_PATH_AFPDUAMPATH=\"${RESDIR}/uams/\" \
	 -D_PATH_AFPDNLSPATH=\"${RESDIR}/nls/\" \
	 -o afp_options.o -c afp_options.c

CFLAGS = @CFLAGS@ -I../../include
