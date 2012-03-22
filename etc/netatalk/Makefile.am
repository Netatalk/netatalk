# Makefile.am for etc/netatlk/

pkgconfdir = @PKGCONFDIR@

sbin_PROGRAMS = netatalk

netatalk_SOURCES = netatalk.c
netatalk_CFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/libevent/include \
	-D_PATH_CONFDIR=\"$(pkgconfdir)/\" \
	-D_PATH_AFPD=\"$(sbindir)/afpd\" \
	-D_PATH_CNID_METAD=\"$(sbindir)/cnid_metad\"

netatalk_LDADD = \
	$(top_builddir)/libatalk/libatalk.la \
	$(top_builddir)/libevent/libevent.la
