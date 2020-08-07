# Makefile.am for etc/macipgw/

pkgconfdir = @PKGCONFDIR@

sbin_PROGRAMS = macipgw

macipgw_SOURCES = \
	atp_input.c \
	macip.c \
	main.c \
	nbp_lkup_async.c \
	tunnel_bsd.c \
	tunnel_linux.c \
	util.c

macipgw_LDADD =  \
	$(top_builddir)/libatalk/libatalk.la

macipgw_LDFLAGS = -export-dynamic 

macipgw_CFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/sys \
	-I$(top_srcdir)/libatalk/atp \
	-DSERVERTEXT=\"$(SERVERTEXT)/\"

noinst_HEADERS = \
	atp_input.h \
	common.h \
	macip.h \
	nbp_lkup_async.h \
	tunnel.h \
	util.h
