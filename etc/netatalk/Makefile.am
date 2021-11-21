# Makefile.am for etc/netatlk/

pkgconfdir = @PKGCONFDIR@

sbin_PROGRAMS = netatalk

netatalk_SOURCES = netatalk.c afp_avahi.c afp_mdns.c afp_zeroconf.c
netatalk_CFLAGS = \
	@ZEROCONF_CFLAGS@ \
	-D_PATH_CONFDIR=\"$(pkgconfdir)/\" \
	-D_PATH_STATEDIR='"$(localstatedir)/netatalk/"' \
	-D_PATH_AFPD=\"$(sbindir)/afpd\" \
	-D_PATH_CNID_METAD=\"$(sbindir)/cnid_metad\"

netatalk_LDADD = \
	@MYSQL_LIBS@ \
	@ZEROCONF_LIBS@ \
	$(top_builddir)/libatalk/libatalk.la

netatalk_LDFLAGS =

netatalk_CFLAGS += @LIBEVENT_CFLAGS@
netatalk_LDFLAGS += @LIBEVENT_LDFLAGS@ -levent

noinst_HEADERS = afp_zeroconf.h afp_avahi.h afp_mdns.h
