# Makefile.am for etc/uams/

SUBDIRS = uams_krb4

if USE_DHX
DHX_UAMS_BASE = uams_dhx_passwd.so
DHX_LINKS = uams_dhx.so
if USE_PGP
DHX_UAMS = $(DHX_UAMS_BASE) uams_pgp.so
else
DHX_UAMS = $(DHX_UAMS_BASE)
endif
endif

if BUILD_PAM
PAM_UAMS= uams_pam.so
if USE_DHX
DHX_PAM_UAMS = uams_dhx_pam.so
endif
endif

if USE_PAM_SO
UAM_CLRTXT_SO=uams_pam.so
UAM_DHX_SO=uams_dhx_pam.so
else
UAM_CLRTXT_SO=uams_passwd.so
UAM_DHX_SO=uams_dhx_passwd.so
endif

uamsdir = @UAMS_PATH@
uams_DATA = $(DHX_UAMS) $(DHX_PAM_UAMS) $(PAM_UAMS) uams_guest.so \
	uams_passwd.so \
	uams_randnum.so

uams_LINKS = $(DHX_LINKS) uams_clrtxt.so

EXTRA_DIST = uams_dhx_pam.c uams_dhx_passwd.c uams_pgp.c \
	uams_guest.c uams_pam.c uams_passwd.c uams_randnum.c

#
# standard library compilation:
#

SUFFIXES = .c .so
.c.so:
	$(COMPILE) -DHAVE_CONFIG_H -fPIC -DPIC \
	  -I$(top_srcdir) -fomit-frame-pointer -c $< ; \
	$(LD) -shared $(LDSHAREDFLAGS) -o $@ $*.o $(LIBS)

#
# create symbolic links:
#

uams_clrtxt.so:
	$(LN_S) -f $(UAM_CLRTXT_SO) $(DESTDIR)$(UAMS_PATH)/uams_clrtxt.so

uams_dhx.so:
	$(LN_S) -f $(UAM_DHX_SO) $(DESTDIR)$(UAMS_PATH)/uams_dhx.so

#
# install/install-strip:
#

install-data-local: $(uams_LINKS)

install-strip:
	strip $(uams_DATA); \
	$(MAKE) $(AM_MAKEFLAGS) install

#
# cleanup
#

CLEANFILES += *.o $(uams_DATA)
