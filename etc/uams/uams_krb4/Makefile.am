# Makefile.am for etc/uams/uams_krb4/

if USE_KERBEROS
KERBEROS_MODULE = uams_krb4.la
else
KERBEROS_MODULE =
endif

uams_krb4_la_SOURCES = kuam.c lifetime.c send_to_kdc.c uams_krb4.c
uams_krb4_la_LDFLAGS = -module -avoid-version

uamsdir = @UAMS_PATH@
uams_LTLIBRARIES = $(KERBEROS_MODULE)
