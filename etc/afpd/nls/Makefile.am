# Makefile.am for etc/afpd/nls/

pkgconfdir = @PKGCONFDIR@
nlsdir = $(pkgconfdir)/nls

bin_PROGRAMS = makecode parsecode

makecode_SOURCES = makecode.c
parsecode_SOURCES = parsecode.c

CFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/sys -I$(top_srcdir)/etc/afpd \
	@CFLAGS@

#
# install/install-strip:
#

install-data-local: makecode
	./makecode; \
	mkdir -p $(DESTDIR)$(nlsdir); \
	$(INSTALL_DATA) maccode.* $(DESTDIR)$(nlsdir)

#
# cleanup
#

CLEANFILES = maccode.*
