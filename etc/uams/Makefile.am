# Makefile.am for etc/uams/

#SUBDIRS = uams_krb4

uams_LTLIBRARIES = uams_dhx_pam.la uams_dhx.passwd.la uams_guest.la uams_pam.la uams_pgp.la uams_randnum.la

uams_dhx_pam_la_SOURCES = uams_dhx_pam.c
uams_dhx_passwd_la_SOURCES = uams_dhx_passwd.c
uams_guest_la_SOURCES = uams_guest.c
uams_pam_la_SOURCES = uams_pam.c
uams_pgp_la_SOURCES = uams_pgp.c
uams_randnum_la_SOURCES = uams_randnum.c

CFLAGS = @CFLAGS@ -I../../include
