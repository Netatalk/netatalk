# Makefile.am for etc/afpd/nls/

pkgconfdir = @PKGCONFDIR@
nlsdir = @NLSDIR@

bin_PROGRAMS = makecode parsecode

makecode_SOURCES = makecode.c
parsecode_SOURCES = parsecode.c

MACCODES = 				\
	maccode.437			\
	maccode.850			\
	maccode.iso8859-1		\
	maccode.iso8859-1.adapted	\
	maccode.koi8-r

INCLUDES = -I$(top_srcdir)/sys -I$(top_srcdir)/etc/afpd

$(MACCODES): makecode
	./makecode $@

#
# install/install-strip:
#

install-data-local: $(MACCODES)
	mkdir -p $(DESTDIR)$(nlsdir); \
	for f in $(MACCODES); do \
		$(INSTALL_DATA) $$f $(DESTDIR)$(nlsdir)/; \
	done

#
# cleanup
#

CLEANFILES = $(MACCODES)
