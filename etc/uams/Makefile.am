# Makefile.am for etc/uams/

SUBDIRS = uams_krb4

#
# conditionally build some modules
#

UAMS_GENERIC = uams_guest.la uams_passwd.la

if USE_DHX
UAMS_DHX_GENERIC = uams_randnum.la uams_dhx_passwd.la
endif

if USE_PGP
UAMS_PGP = uams_pgp.la
endif

if BUILD_PAM
UAMS_PAM = uams_pam.la
if USE_DHX
UAMS_DHX_PAM = uams_dhx_pam.la
endif
endif

if USE_GSSAPI
UAMS_GSSAPI = uams_gss.la
endif

# links
if USE_PAM_SO
UAMS_CLRTXT_LINK = uams_pam.so
UAMS_DHX_LINK    = uams_dhx_pam.so
else
UAMS_CLRTXT_LINK = uams_passwd.so
UAMS_DHX_LINK    = uams_dhx_passwd.so
endif

#
# source files
#

uams_guest_la_SOURCES      = uams_guest.c
uams_randnum_la_SOURCES    = uams_randnum.c
uams_passwd_la_SOURCES     = uams_passwd.c
uams_pam_la_SOURCES        = uams_pam.c
uams_pgp_la_SOURCES        = uams_pgp.c
uams_dhx_passwd_la_SOURCES = uams_dhx_passwd.c
uams_dhx_pam_la_SOURCES    = uams_dhx_pam.c
uams_gss_la_SOURCES   = uams_gss.c

#
# flags
#

# do that on behalf of the brokeness of automake 1.4
AM_CFLAGS = @CFLAGS@ @SSL_CFLAGS@

uams_guest_la_CFLAGS      = @CFLAGS@
uams_randnum_la_CFLAGS    = @CFLAGS@ @SSL_CFLAGS@
uams_passwd_la_CFLAGS     = @CFLAGS@
uams_pam_la_CFLAGS        = @CFLAGS@ @PAM_CFLAGS@
uams_pgp_la_CFLAGS        = @CFLAGS@ @SSL_CFLAGS@
uams_dhx_passwd_la_CFLAGS = @CFLAGS@ @SSL_CFLAGS@
uams_dhx_pam_la_CFLAGS    = @CFLAGS@ @SSL_CFLAGS@ @PAM_CFLAGS@
uams_gss_la_CFLAGS   	  = @CFLAGS@ @GSSAPI_CFLAGS@

uams_guest_la_LDFLAGS      = -module -avoid-version
uams_randnum_la_LDFLAGS    = -module -avoid-version @SSL_LIBS@
uams_passwd_la_LDFLAGS     = -module -avoid-version @CRYPT_LIBS@
uams_pam_la_LDFLAGS        = -module -avoid-version @PAM_LIBS@
uams_pgp_la_LDFLAGS        = -module -avoid-version @SSL_LIBS@
uams_dhx_passwd_la_LDFLAGS = -module -avoid-version @CRYPT_LIBS@ @SSL_LIBS@
uams_dhx_pam_la_LDFLAGS    = -module -avoid-version @SSL_LIBS@ @PAM_LIBS@
uams_gss_la_LDFLAGS   	   = -module -avoid-version @GSSAPI_LIBS@ 

#
# module compilation
#

uamsdir = @UAMS_PATH@
uams_LTLIBRARIES =		\
	$(UAMS_GENERIC)		\
	$(UAMS_PGP)		\
	$(UAMS_PAM)		\
	$(UAMS_DHX_GENERIC)	\
	$(UAMS_DHX_PAM)		\
	$(UAMS_GSSAPI)

#
# link creation
#

uams_LINKS =

if USE_DHX
install-exec-hook:
	(cd $(DESTDIR)$(uamsdir)					&& \
		rm -f uams_clrtxt.so uams_dhx.so			&& \
		$(LN_S) $(UAMS_CLRTXT_LINK) uams_clrtxt.so		&& \
		$(LN_S) $(UAMS_DHX_LINK) uams_dhx.so			   \
	)

uninstall-hook:
	(cd $(DESTDIR)$(uamsdir)					&& \
		rm -f uams_clrtxt.so uams_dhx.so			   \
	)
else
install-exec-hook:
	(cd $(DESTDIR)$(uamsdir)					&& \
		rm -f uams_clrtxt.so					&& \
		$(LN_S) $(UAMS_CLRTXT_LINK) uams_clrtxt.so		   \
	)

uninstall-hook:
	(cd $(DESTDIR)$(uamsdir)					&& \
		rm -f uams_clrtxt.so 					   \
	)
endif
