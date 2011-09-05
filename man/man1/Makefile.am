# Makefile.am for man/man1/

pkgconfdir = @PKGCONFDIR@

SUFFIXES= .tmpl .

.tmpl:
	sed -e s@:SBINDIR:@${sbindir}@ \
	    -e s@:BINDIR:@${bindir}@ \
	    -e s@:ETCDIR:@${pkgconfdir}@ \
	    -e s@:LIBDIR:@${libdir}@ \
	    -e s@:DEFAULT_CNID_SCHEME:@${DEFAULT_CNID_SCHEME}@ \
	    <$< >$@

GENERATED_MANS	= uniconv.1 asip-status.pl.1 afpldaptest.1
TEMPLATE_FILES	= uniconv.1.tmpl asip-status.pl.1.tmpl afpldaptest.1.tmpl
NONGENERATED_MANS	=	ad.1 \
				afppasswd.1 \
				apple_dump.1 \
				dbd.1 \
				hqx2bin.1 \
				macbinary.1 \
				macusers.1 \
				megatron.1 \
				netatalk-config.1 \
				single2bin.1 \
				unbin.1 \
				unhex.1 \
				unsingle.1
ATALK_MANS = aecho.1 \
				getzones.1 \
				nbp.1 \
				nbplkup.1 \
				nbprgstr.1 \
				nbpunrgstr.1 \
				pap.1 \
				papstatus.1 \
				psorder.1

if USE_APPLETALK
NONGENERATED_MANS += $(ATALK_MANS)
endif

man_MANS = $(GENERATED_MANS) $(NONGENERATED_MANS)
CLEANFILES = $(GENERATED_MANS)
EXTRA_DIST = $(TEMPLATE_FILES) $(NONGENERATED_MANS) $(ATALK_MANS)
