# Makefile.am for etc/uams/

if USE_DHX
DHX_UAMS = uams_dhx_pam.so uams_dhx_passwd.so
endif

uamsdir = $(CONFIG_DIR)/uams
uams_DATA = $(DHX_UAMS) uams_guest.so uams_pam.so uams_passwd.so uams_randnum.so

EXTRA_DIST = uams_dhx_pam.c uams_dhx_passwd.c \
	uams_guest.c uams_pam.c uams_passwd.c uams_randnum.c

SUFFIXES = .c .so
.c.so:
	$(COMPILE) -DHAVE_CONFIG_H -fPIC -DPIC \
	  -I$(top_srcdir) -fomit-frame-pointer -c $< ; \
	$(LD) -shared -o $@ $*.o $(LIBS)

clean distclean clean-recursive distclean-recursive:
	$(RM) $(uams_DATA) *.o core

