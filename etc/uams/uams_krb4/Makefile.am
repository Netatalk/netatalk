# Makefile.am for etc/uams/uams_krb4/

if USE_KERBEROS

uamsdir = @UAMS_PATH@
uams_DATA = uams_krb4.so

endif

EXTRA_DIST = kuam.c lifetime.c send_to_kdc.c uams_krb4.c

#
# standard library compilation:
#

SUFFIXES = .c .o

if USE_KERBEROS

.c.o:
	$(COMPILE) -DHAVE_CONFIG_H -fPIC -DPIC \
	  -I$(top_srcdir) -fomit-frame-pointer -c $<

uams_krb4.so: kuam.o lifetime.o sent_to_kdc.o uams_krb4.o
	$(LD) -shared $(LDSHAREDFLAGS) -o $@ *.o $(LIBS)

#
# install/install-strip:
#

install-strip:
	strip $(uams_DATA); \
	$(MAKE) $(AM_MAKEFLAGS) install

endif

#
# cleanup
#

clean distclean clean-recursive distclean-recursive:
	$(RM) $(uams_DATA) *.o core

