# Makefile.am for etc/papd/

sbin_PROGRAMS = papd

papd_SOURCES = main.c printcap.c session.c file.c comment.c lp.c ppd.c magics.c headers.c queries.c auth.c uam.c
papd_LDADD = ../../libatalk/libatalk.a -latalk

noinst_HEADERS = comment.h file.h ppd.h printer.h uam_auth.h

CFLAGS = @CFLAGS@ -I../../include -I../../sys \
	-D_PATH_PAPDCONF=\"$(CONFIG_DIR)/papd.conf\" \
	-D_PATH_PAPDUAMPATH=\"$(UAMS_PATH)/\" \
	-DVERSION=\"$(NETATALK_VERSION)\"
