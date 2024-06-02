# Makefile.am for man/man1/

GENERATED_MANS	=	ad.1 \
			aecho.1 \
			afpldaptest.1 \
			afppasswd.1 \
			apple_dump.1 \
			asip-status.1 \
			cnid2_create.1 \
			dbd.1 \
			macusers.1 \
			megatron.1 \
			netatalk-config.1 \
			showppd.1 \
			uniconv.1

STATIC_MANS	=	binheader.1 \
			hqx2bin.1 \
			macbinary.1 \
			nadheader.1 \
			single2bin.1 \
			unbin.1 \
			unhex.1 \
			unsingle.1

if USE_APPLETALK
GENERATED_MANS	+=	aecho.1 \
			getzones.1 \
			nbp.1 \
			pap.1 \
			psorder.1

STATIC_MANS	+=	nbplkup.1 \
			nbprgstr.1 \
			nbpunrgstr.1 \
			papstatus.1
endif

man_MANS = $(GENERATED_MANS) $(STATIC_MANS)
DISTCLEANFILES = $(GENERATED_MANS)
EXTRA_DIST = $(STATIC_MANS)
