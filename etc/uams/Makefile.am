# Makefile.am for etc/uams/

#SUBDIRS = uams_krb4

uamsdir = $(UAMS_PATH)
uams_LTLIBRARIES = lib_uams_dhx_pam.la lib_uams_dhx.passwd.la lib_uams_guest.la lib_uams_pam.la lib_uams_randnum.la
# removed lib_uams_pgp.la since it doesn't currently build (AFAIK)

lib_uams_dhx_pam_la_SOURCES = uams_dhx_pam.c
lib_uams_dhx_passwd_la_SOURCES = uams_dhx_passwd.c
lib_uams_guest_la_SOURCES = uams_guest.c
lib_uams_pam_la_SOURCES = uams_pam.c
#lib_uams_pgp_la_SOURCES = uams_pgp.c
lib_uams_randnum_la_SOURCES = uams_randnum.c

CFLAGS = @CFLAGS@ -I$(top_srcdir)/include -I$(top_srcdir)/sys \
	-DUAM_RNDNUM
