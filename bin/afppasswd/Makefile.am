# Makefile.am for bin/afppasswd/

bin_PROGRAMS = afppasswd

afppasswd_SOURCES = afppasswd.c
afppasswd_LDADD = $(top_srcdir)/libatalk/libatalk.a

CFLAGS = @CFLAGS@ \
	-D_PATH_AFPDPWFILE=\"$(CONFIG_DIR)/afppasswd\"
