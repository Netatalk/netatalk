# Makefile.am for etc/uams/

if USE_DHX
DHX_UAMS = uams_dhx_pam.so uams_dhx_passwd.so uams_dhx.so
endif

uamsdir = $(CONFIG_DIR)/uams
uams_DATA = $(DHX_UAMS) uams_guest.so \
	uams_pam.so uams_passwd.so \
	uams_randnum.so uams_clrtxt.so

EXTRA_DIST = uams_dhx_pam.c uams_dhx_passwd.c \
	uams_guest.c uams_pam.c uams_passwd.c uams_randnum.c

#
# standard library compilation:
#

SUFFIXES = .c .so
.c.so:
	$(COMPILE) -DHAVE_CONFIG_H -fPIC -DPIC \
	  -I$(top_srcdir) -fomit-frame-pointer -c $< ; \
	$(LD) -shared -o $@ $*.o $(LIBS)

#
# create symbolic links:
#

uams_clrtxt.so:
	$(LN_S) uams_pam.so uams_clrtxt.so

uams_dhx.so:
	$(LN_S) uams_dhx_pam.so uams_dhx.so

#
# install/install-strip:
#

install:
	mkdir -p $(DESTDIR)$(UAMS_PATH); \
	rm -f $(DESTDIR)$(UAMS_PATH)/uams_{clrtxt,dhx}.so; \
	cp -dpf $(uams_DATA) $(DESTDIR)$(UAMS_PATH)/

install-strip:
	strip $(uams_DATA); \
	$(MAKE) $(AM_MAKEFLAGS) install

#
# cleanup
#

clean distclean clean-recursive distclean-recursive:
	$(RM) $(uams_DATA) *.o core
