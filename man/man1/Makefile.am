# Makefile.am for man/man1/

pkgconfdir = @PKGCONFDIR@

SUFFIXES= .tmpl .

.tmpl:
	sed -e s@:SBINDIR:@${sbindir}@ \
	    -e s@:BINDIR:@${bindir}@ \
	    -e s@:ETCDIR:@${pkgconfdir}@ \
	    -e s@:LIBDIR:@${libdir}@ \
	    <$< >$@

GENERATED_MANS	= apple_cp.1 apple_mv.1 apple_rm.1
TEMPLATE_FILES	= $(foreach f,$(GENERATED_MANS),$(f).tmpl)
NONGENERATED_MANS	=	achfile.1  \
				acleandir.1 \
				aecho.1 \
				afile.1 \
				afppasswd.1 \
				getzones.1 \
				hqx2bin.1 \
				macbinary.1 \
				megatron.1 \
				nbp.1 \
				nbplkup.1 \
				nbprgstr.1 \
				nbpunrgstr.1 \
				netatalk-config.1 \
				pap.1 \
				papstatus.1 \
				psorder.1 \
				single2bin.1 \
				timeout.1 \
				unbin.1 \
				unhex.1 \
				unsingle.1

man_MANS = $(GENERATED_MANS) $(NONGENERATED_MANS)

CLEANFILES = $(GENERATED_MANS)

EXTRA_DIST = $(TEMPLATE_FILES) $(NONGENERATED_MANS)

