# Makefile.am for etc/afpd/nls/

EXTRA_DIST = maccode.437 maccode.850 maccode.iso8859-1 
bin_PROGRAMS = makecode parsecode

makecode_SOURCES = makecode.c
parsecode_SOURCES = parsecode.c

CFLAGS = @CFLAGS@ -I$(top_builddir)/include -I$(top_builddir)/sys -I..

#
# install/install-strip:
#

install-data-local: makecode
	./makecode; \
	mkdir -p $(DESTDIR)@CONFIG_DIR@/nls; \
	$(INSTALL) maccode.* $(DESTDIR)@CONFIG_DIR@/nls/

#
# cleanup
#

clean distclean clean-recursive distclean-recursive:
	$(RM) maccode.* *.o core makecode parsecode

