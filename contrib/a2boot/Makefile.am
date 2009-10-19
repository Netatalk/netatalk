# Makefile.am for contrib/a2boot/

sbin_PROGRAMS = a2boot

a2boot_SOURCES = a2boot.c
a2boot_LDADD = $(top_builddir)/libatalk/libatalk.la

LIBS = @LIBS@

EXTRA_DIST = COPYRIGHT VERSION

AM_CFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/sys \
	 @SLP_CFLAGS@ \
	 -D_PATH_A_GS_BLOCKS=\"$(PKGCONFDIR)/a2boot/ProDOS16\ Boot\ Blocks\" \
	 -D_PATH_A_2E_BLOCKS=\"$(PKGCONFDIR)/a2boot/Apple\ :2f:2fe\ Boot\ Blocks\" \
	 -D_PATH_P16_IMAGE=\"$(PKGCONFDIR)/a2boot/ProDOS16\ Image\"
