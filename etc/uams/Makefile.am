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

#
# flags
#

INCLUDES = @SSL_CFLAGS@

uams_guest_la_CFLAGS      = @CFLAGS@
uams_randnum_la_CFLAGS    = @CFLAGS@ @SSL_CFLAGS@
uams_passwd_la_CFLAGS     = @CFLAGS@
uams_pam_la_CFLAGS        = @CFLAGS@
uams_pgp_la_CFLAGS        = @CFLAGS@ @SSL_CFLAGS@
uams_dhx_passwd_la_CFLAGS = @CFLAGS@ @SSL_CFLAGS@
uams_dhx_pam_la_CFLAGS    = @CFLAGS@ @SSL_CFLAGS@

uams_guest_la_LDFLAGS      = -module -avoid-version
uams_randnum_la_LDFLAGS    = -module -avoid-version @SSL_LIBS@
uams_passwd_la_LDFLAGS     = -module -avoid-version
uams_pam_la_LDFLAGS        = -module -avoid-version -lpam
uams_pgp_la_LDFLAGS        = -module -avoid-version @SSL_LIBS@
uams_dhx_passwd_la_LDFLAGS = -module -avoid-version @SSL_LIBS@
uams_dhx_pam_la_LDFLAGS    = -module -avoid-version @SSL_LIBS@ -lpam

#
# module compilation
#

uamsdir = @UAMS_PATH@
uams_LTLIBRARIES =		\
	$(UAMS_GENERIC)		\
	$(UAMS_PGP)		\
	$(UAMS_PAM)		\
	$(UAMS_DHX_GENERIC)	\
	$(UAMS_DHX_PAM)

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
else
install-exec-hook:
	(cd $(DESTDIR)$(uamsdir)					&& \
		rm -f uams_clrtxt.so					&& \
		$(LN_S) $(UAMS_CLRTXT_LINK) uams_clrtxt.so		   \
	)
endif
